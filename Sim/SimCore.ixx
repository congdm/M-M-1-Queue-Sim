#include <stdexcept>
#include <queue>
#include <algorithm>

export module SimCore;
import MathMod;

namespace SimCore {
	void assert(bool cond) {
		if (!cond)
			throw std::runtime_error("assertion failed");
	}

	/* ----------------------------------------------------------------------------- */
	/* Type definitions */

	#define RANDOM_ARRAY_SIZE 131072

	class Sim;
	class Node;
	enum Message { arrive, done };
	class Packet;

	class Event {
	public:
		double t;
		Node* receiver;
		Message action;
		Packet* pkt;

		Event();
		static void init(Event& ev, Node* rec, Message act, double t, Packet* p);
	};

	class EventList {
	private:
		Event* events;
		int n, maxlen;

		void sift_up(int x);
		void heapify(int x); 
	public:
		EventList();
		void put(Event ev);
		bool get(Event& ev);
		bool notEmpty();
	};

	class Packet {
	public:
		int id;
		double creationTime, totalServTime, totalQueueTime;
	};
	
	class Node {
	protected:
		Sim* sim;
	public:
		virtual void handle(Message msg, double t, Packet* pkt) {}
	};

	class GenNode : public Node {
	private:
		double lambda;
		double rand[RANDOM_ARRAY_SIZE];
		unsigned int ridx;
		double t;
		Node* outNode;

		double getInverval();
	public:
		GenNode(Sim* sim, double lambda);
		void set_output(Node* out);
		void generate();
		double getTime();
	};

	class SinkNode : public Node {
	public:
		void handle(Message msg, double t, Packet* pkt);
		SinkNode(Sim* sim);
	};

	class BasicNode : public Node {
	private:
		double mu;
		double rand[RANDOM_ARRAY_SIZE];
		unsigned int ridx;
		double t;
		std::queue<Packet*> queue;
		int queueLen, nInQueue;
		bool busyState;
		Node* outNode;

		double getServiceTime();
	public:
		BasicNode(Sim* sim, double mu, int queueLen);
		void set_output(Node* out);
		void handle(Message msg, double evTime, Packet* pkt);
	};

	export class Sim {
	private:
		double genMinT, genMaxT;

		void generateUntil(double threshold);
		void resetStateVars(int limit);
	public:
		int nDrop;
		double avgTime;
		double* servTimeData, * queueTimeData;
		bool* droppedPktData;
		int dataCnt;
		int gid;
		GenNode** gen;
		BasicNode** node;
		SinkNode* sink;
		int nGen, nNode;
		EventList evList;

		Sim();
		void init(int n, double lambda[], double mu[], int queueLen[]);
		void runSim(int limit);
		void cleanUp();
	};

	/* ----------------------------------------------------------------------------- */
	/* Functions of Event class */

	Event::Event() {
	}

	void Event::init(Event& ev, Node* rec, Message act, double t, Packet* p) {
		ev.receiver = rec;
		ev.action = act;
		ev.pkt = p;
		ev.t = t;
	}

	/* ----------------------------------------------------------------------------- */
	/* Functions of EventList class */

	void EventList::sift_up(int x) {
		int y;
		while (x > 0) {
			if (x % 2 == 0) y = x / 2 - 1;
			else {
				y = x / 2;
			}
			if (events[y].t > events[x].t) {
				Event temp;
				temp = events[x]; events[x] = events[y]; events[y] = temp;
			}
			x = y;
		}
	}

	void EventList::heapify(int x) {
		int left, right;
		left = x * 2 + 1; right = x * 2 + 2;
		if (left < n && events[left].t < events[x].t) {
			Event temp;
			temp = events[x]; events[x] = events[left]; events[left] = temp;
			heapify(left);
		}
		else if (right < n && events[right].t < events[x].t) {
			Event temp;
			temp = events[x]; events[x] = events[right]; events[right] = temp;
			heapify(right);
		}
	}
	
	EventList::EventList() {
		events = new Event[128];
		n = 0;
		maxlen = 128;
	}

	void EventList::put(Event ev) {
		if (n >= maxlen) {
			assert(n == maxlen);
			Event* old;
			old = events; events = new Event[maxlen*2];
			std::copy(old, old + maxlen, events);
			maxlen = maxlen * 2; delete old;
		}
		events[n] = ev; sift_up(n); n++;
	}

	bool EventList::get(Event &ev) {
		bool success;
		if (n > 0) {
			ev = events[0];
			if (n - 1 > 0) {
				events[0] = events[n - 1]; n--; heapify(0);
			}
			else {
				n--; assert(n == 0);
			}
			success = true;
		}
		else {
			success = false;
		}
		return success;
	}

	bool EventList::notEmpty() {
		return (n > 0);
	}

	/* ----------------------------------------------------------------------------- */
	/* Misc */

	void updatePacketLog(Sim* sim, Packet* pkt) {
		int pktId = pkt->id;
		if (pktId < sim->dataCnt) {
			sim->servTimeData[pktId] = pkt->totalServTime;
			sim->queueTimeData[pktId] = pkt->totalQueueTime;
		}
	}

	void dropPacket(Sim* sim, Node* node, Packet* pkt, double t) {
		int pktId = pkt->id;
		if (pktId < sim->dataCnt) {
			sim->droppedPktData[pktId] = true;
			updatePacketLog(sim, pkt);
		}
		delete pkt;
		sim->nDrop++;
	}

	/* ----------------------------------------------------------------------------- */
	/* Functions of GenNode class */

	double GenNode::getInverval() {
		if (this->ridx >= RANDOM_ARRAY_SIZE) {
			MathMod::gen_exponential(this->rand, this->lambda, RANDOM_ARRAY_SIZE);
			this->ridx = 0;
		}
		double interval = this->rand[this->ridx];
		this->ridx++;
		return interval;
	}

	GenNode::GenNode(Sim* sim,  double lambda) {
		this->lambda = lambda;
		this->sim = sim;
		MathMod::gen_exponential(this->rand, lambda, RANDOM_ARRAY_SIZE);
		this->ridx = 0;
		this->t = 0;
	}

	void GenNode::set_output(Node* out) {
		this->outNode = out;
	}

	void GenNode::generate() {
		double interval;
		Packet* pkt;
		Event ev;

		if (this->lambda > 0.0) {
			interval = this->getInverval();
			this->t = this->t + interval;

			pkt = new Packet();
			pkt->id = sim->gid;
			sim->gid++;
			pkt->creationTime = t;
			pkt->totalQueueTime = 0;
			pkt->totalServTime = 0;

			Event::init(ev, this->outNode, arrive, t, pkt);
			sim->evList.put(ev);
		}
	}

	double SimCore::GenNode::getTime()
	{
		return this->t;
	}

	/* ----------------------------------------------------------------------------- */
	/* Functions of SinkNode class */

	SinkNode::SinkNode(Sim* sim) {
		this->sim = sim;
	}

	void SinkNode::handle(Message msg, double t, Packet* pkt) {
		assert(msg == arrive);
		/* do something */
		updatePacketLog(sim, pkt);
		printf("Packet %d: created at %.4f arrived to sink at %.4f\n", pkt->id, pkt->creationTime, t);
		delete pkt;
	}

	/* ----------------------------------------------------------------------------- */
	/* Functions of BasicNode class */

	double BasicNode::getServiceTime() {
		if (this->ridx >= std::size(this->rand)) {
			MathMod::gen_exponential(this->rand, this->mu, RANDOM_ARRAY_SIZE);
			this->ridx = 0;
		}
		double servTime = this->rand[this->ridx];
		this->ridx++;
		return servTime;
	}

	BasicNode::BasicNode(Sim* sim, double mu, int queueLen) {
		this->mu = mu;
		this->sim = sim;
		MathMod::gen_exponential(this->rand, mu, RANDOM_ARRAY_SIZE);
		this->ridx = 0;
		this->t = 0;
		this->queueLen = queueLen;
		this->nInQueue = 0;
		this->busyState = false;
	}

	void BasicNode::set_output(Node* out) {
		this->outNode = out;
	}

	void BasicNode::handle(Message msg, double evTime, Packet* pkt) {
		if (msg == arrive) {
			if (!this->busyState) {
				/* Available */
				double serviceTime = this->getServiceTime();
				this->t = evTime + serviceTime;
				pkt->totalServTime = pkt->totalServTime + serviceTime;

				Event ev;
				Event::init(ev, this, done, this->t, pkt);
				this->sim->evList.put(ev);
				this->busyState = true;
			}
			else if (this->nInQueue < this->queueLen) {
				/* Put into queue */
				this->queue.push(pkt);
				this->nInQueue++;
				pkt->totalQueueTime = pkt->totalQueueTime - evTime;
			}
			else {
				/* Drop packet */
				dropPacket(sim, this, pkt, evTime);
			}
		}
		else if (msg == done) {
			assert(this->t == evTime);
			Event ev;
			Event::init(ev, this->outNode, arrive, evTime, pkt);
			sim->evList.put(ev);

			/* if queue is not empty */
			if (this->nInQueue > 0) {
				Packet* pkt2 = this->queue.front();
				this->queue.pop();
				this->nInQueue--;
				pkt2->totalQueueTime = pkt2->totalQueueTime + evTime;

				double serviceTime = this->getServiceTime();
				this->t = this->t + serviceTime;
				pkt2->totalServTime = pkt2->totalServTime + serviceTime;

				Event ev;
				Event::init(ev, this, done, this->t, pkt2);
				sim->evList.put(ev);
			}
			else {
				/* node is available now */
				this->busyState = false;
			}
		}
		else
			assert(false);
	}

	/* ----------------------------------------------------------------------------- */
	/* Functions of Sim class */

	Sim::Sim() {
		servTimeData = nullptr;
		queueTimeData = nullptr;
		droppedPktData = nullptr;

		gen = nullptr;
		node = nullptr;
		sink = nullptr;
	}

	/* POSTCONDITION: this procedure ensures all possible packets
	   before and at threshold time have been generated */
	void Sim::generateUntil(double threshold) {
		while (genMinT <= threshold) {
			for (int i = 0; i < nGen; i++) {
				gen[i]->generate();
				double t = gen[i]->getTime();
				if (i > 0) {
					if (t < genMinT) genMinT = t;
					else if (t > genMaxT) genMaxT = t;
				}
				else {
					genMinT = t; genMaxT = t;
				}
			}
		}
	}

	void Sim::resetStateVars(int limit)
	{
		nDrop = 0;
		avgTime = 0;
		dataCnt = limit;
		delete servTimeData; delete queueTimeData; delete droppedPktData;
		servTimeData = new double[limit];
		queueTimeData = new double[limit];
		droppedPktData = new bool[limit](); /* zero initialized */

		gid = 0;
		genMinT = 0;
		genMaxT = 0;
	}

	/* Init a linear network of Poisson flow generators and nodes with exponential service time */
	void Sim::init(int n, double lambda[], double mu[], int queueLen[]) {
		int i;

		gen = new GenNode * [n];
		node = new BasicNode * [n];
		nGen = n; nNode = n;
		sink = new SinkNode(this);
		for (i = n-1; i >= 0; i--) {
			gen[i] = new GenNode(this, lambda[i]);
			node[i] = new BasicNode(this, mu[i], queueLen[i]);
			gen[i]->set_output(node[i]);
			if (i == n - 1) node[i]->set_output(sink);
			else node[i]->set_output(node[i + 1]);
		}
	}

	void Sim::runSim(int limit) {
		Event ev;

		resetStateVars(limit);
		generateUntil(0);
		do {
			evList.get(ev);
			if (ev.t <= genMinT)
				ev.receiver->handle(ev.action, ev.t, ev.pkt);
			else {
				evList.put(ev);
				double threshold = std::max(genMaxT, ev.t);
				generateUntil(threshold);
				
			}
		} while (gid < limit);

		/* process the remaining events */
		while (evList.notEmpty()) {
			evList.get(ev);
			ev.receiver->handle(ev.action, ev.t, ev.pkt);
		}
	}

	void Sim::cleanUp()	{
		int i;
		for (i = 0; i < nGen; i++) delete gen[i];
		for (i = 0; i < nNode; i++) delete node[i];
		delete gen; delete node; delete sink;
		gen = nullptr; node = nullptr; sink = nullptr;
	}
}