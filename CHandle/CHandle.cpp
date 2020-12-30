// CHandle.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <memory>
#include <vector>
#include <set>

int global_id = 0;

/*****************************************************************************/

class Node
{
public:
	Node();
	~Node();

	int get_id();

	// TODO(Andrea): why do these need to be public?
	virtual double eval() = 0; // TODO(Andrea): read on pure virtual functions
	virtual void post_order(std::set<int>& visited, std::vector<Node*>& topological_order) = 0;

	double get_computed_value();
	void set_computed_value(double computed_value_);

private:
	int id;
	// TODO(Andrea): this should be a pointer to a generic value I believe.
	// Two reasons: to better model the absence of a value and to be able to
	// hold a generic value such as a tensor.
	double computed_value;
};

Node::Node()
{
	this->id = global_id++;
	this->computed_value = 0.0;
}

Node::~Node()
{
}

int Node::get_id() {
	return this->id;
}

double Node::get_computed_value()
{
	return this->computed_value;
}

void Node::set_computed_value(double computed_value_)
{
	this->computed_value = computed_value_;
}

/*****************************************************************************/

class Constant : public Node
{
public:
	Constant(double value_);
	~Constant();

	double eval();
	void post_order(std::set<int>& visited, std::vector<Node*>& topological_order);

	// TODO(Andrea): implement addition syntax sugar. Will it require prototype
	// declarations?

private:
	double value;
};

Constant::Constant(double value_)
{
	this->value = value_;
}

Constant::~Constant()
{
}

double Constant::eval() {
	std::cout << "evaluation id=" << this->get_id() << " value=" << this->value << std::endl;

	this->set_computed_value(this->value);

	return this->get_computed_value();
}

void Constant::post_order(std::set<int>& visited, std::vector<Node*>& topological_order) {
	visited.insert(this->get_id());
	topological_order.push_back(this);
}

/*****************************************************************************/


// TODO(Andrea): shouldn't the fact that we are using shared pointers be an
// implementation detail?
class Addition : public Node
{
public:
	Addition(std::shared_ptr<Node> lhs_, std::shared_ptr<Node> rhs_);
	~Addition();

	double eval();
	void post_order(std::set<int>& visited, std::vector<Node*>& topological_order);

private:
	std::shared_ptr<Node> lhs;
	std::shared_ptr<Node> rhs;
};

// TODO(Andrea): read up on short var initialization syntax
Addition::Addition(std::shared_ptr<Node> lhs_, std::shared_ptr<Node> rhs_)
{
	this->lhs = lhs_;
	this->rhs = rhs_;
}

Addition::~Addition()
{
}

double Addition::eval()
{
	// NOTE: evaluation order in a() + b() is undefined. This is why we use
	// temporary variables
	auto lhs_val = lhs->get_computed_value();
	auto rhs_val = rhs->get_computed_value();

	this->set_computed_value(lhs_val + rhs_val);

	std::cout << "evaluation id=" << this->get_id()
		<< " value=" << this->get_computed_value() << std::endl;

	return this->get_computed_value();
}

// TODO(Andrea): I am making copies
void Addition::post_order(std::set<int>& visited, std::vector<Node*>& topological_order)
{
	visited.insert(this->get_id());
	if (visited.find(this->lhs->get_id()) == visited.end()) this->lhs->post_order(visited, topological_order);
	if (visited.find(this->rhs->get_id()) == visited.end()) this->rhs->post_order(visited, topological_order);

	// ok this should make a copy, TODO(Andrea): check it
	// TODO(Andrea): is shared_ptr<A> contravariant to A -> B?
	// TODO(Andrea): is this naked pointer an anti-pattern?
	topological_order.push_back(this);
}


/*****************************************************************************/

int main()
{
	using std::cout; using std::make_shared; using std::endl;
	// Create a new Node and a new shared pointer. Then put the node in the shared
	// pointer.
	auto two = make_shared<Constant>(2);
	auto three = make_shared<Constant>(3);

	// Create another reference to the node, using shared_ptr's copy constructor.
	//auto twoRef(two);
	//std::cout << "twoRef->getId()=" << twoRef->getId() << std::endl;
	//std::cout << "twoRef->eval()=" << twoRef->eval() << std::endl;

	auto add1 = make_shared<Addition>(two, three);
	auto four = make_shared<Constant>(4);
	auto add2 = make_shared<Addition>(add1, four);
	auto add3 = make_shared<Addition>(add2, three);
	cout << "add3->eval()=" << add3->eval() << endl;

	// TODO(Andrea): should I use an hashset rather than a set?
	std::set<int> visited;
	std::vector<Node*> topological_order;
	add3->post_order(visited, topological_order);
	for (auto n : topological_order) n->eval();

	cout << "add3->eval()=" << add3->get_computed_value() << endl;
}

