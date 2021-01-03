// CHandle.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include "CHandle.h"

int global_id = 0;

/*****************************************************************************/

class Node;
class Constant;
class Addition;

/*****************************************************************************/

class Node
{
public:
	int get_id();

	virtual double forward(const std::unordered_map<std::string, double>& env) = 0;

	// TODO(Andrea): I don't like it, but how to make it return a variable number of values?
	virtual std::vector<double> backward() = 0;
	virtual std::vector<std::shared_ptr<Node>> children() = 0;

	double get_computed_value();
	void set_computed_value(double computed_value_);

	double get_adjoint();
	void add_to_adjoint(double computed_value_);

protected:
	// NOTE: the constructor is protected so that (hopefully) it will only be
	// called implicitely by the constructors of the child classes
	Node();

private:
	int id;
	// TODO(Andrea): this should be a pointer to a generic value I believe.
	// Two reasons: to better model the absence of a value and to be able to
	// hold a generic value such as a tensor.
	double computed_value;

	double adjoint;
};

Node::Node() : id(global_id++), computed_value(0.0), adjoint(0.0) {}

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

double Node::get_adjoint()
{
	return this->adjoint;
}

void Node::add_to_adjoint(double adjoint_)
{
	this->adjoint += adjoint_;
}

/*****************************************************************************/

class Constant : public Node, public std::enable_shared_from_this<Constant>
{
public:
	static std::shared_ptr<Constant> make(double value_);

	double forward(const std::unordered_map<std::string, double>& env) override;
	std::vector<double> backward() override;
	std::vector<std::shared_ptr<Node>> children() override;

private:
	Constant(double value_);
	double value;
};

std::shared_ptr<Constant> Constant::make(double value_)
{
	return std::shared_ptr<Constant>(new Constant(value_));
}

Constant::Constant(double value_) : value(value_) {}

double Constant::forward(const std::unordered_map<std::string, double>& env) {
	return this->value;
}
std::vector<double> Constant::backward() {
	return {};
}

std::vector<std::shared_ptr<Node>> Constant::children()
{
	return {};
}

/*****************************************************************************/

class Variable : public Node, public std::enable_shared_from_this<Variable>
{
public:
	static std::shared_ptr<Variable> make(std::string name_);

	double forward(const std::unordered_map<std::string, double>& env) override;
	std::vector<double> backward() override;
	std::vector<std::shared_ptr<Node>> children() override;

private:
	Variable(std::string name_);
	std::string name;
};

std::shared_ptr<Variable>Variable::make(std::string name_)
{
	return std::shared_ptr<Variable>(new Variable(name_));
}

Variable::Variable(std::string name_) : name(name_) {}

double Variable::forward(const std::unordered_map<std::string, double>& env) {
	return env.at(this->name);
}

std::vector<double> Variable::backward() {
	return {};
}


std::vector<std::shared_ptr<Node>> Variable::children()
{
	return {};
}


/*****************************************************************************/


class Addition : public Node, public std::enable_shared_from_this<Addition>
{
public:
	static std::shared_ptr<Addition> make(std::shared_ptr<Node> lhs_, std::shared_ptr<Node> rhs_);

	double forward(const std::unordered_map<std::string, double>& env) override;
	std::vector<double> backward() override;
	std::vector<std::shared_ptr<Node>> children() override;

private:
	Addition(std::shared_ptr<Node> lhs_, std::shared_ptr<Node> rhs_);

	std::shared_ptr<Node> lhs;
	std::shared_ptr<Node> rhs;
};

Addition::Addition(std::shared_ptr<Node> lhs_, std::shared_ptr<Node> rhs_) : lhs(lhs_), rhs(rhs_) {}

std::shared_ptr<Addition> Addition::make(std::shared_ptr<Node> lhs_, std::shared_ptr<Node> rhs_)
{
	return std::shared_ptr<Addition>(new Addition(lhs_, rhs_));
}

double Addition::forward(const std::unordered_map<std::string, double>& env)
{
	// NOTE: evaluation order in a() + b() is undefined. This is why we use
	// temporary variables
	auto lhs_val = lhs->get_computed_value();
	auto rhs_val = rhs->get_computed_value();

	return lhs_val + rhs_val;
}


std::vector<double> Addition::backward() {
	return { 1, 1 };
}

std::vector<std::shared_ptr<Node>> Addition::children()
{
	return { this->lhs, this->rhs };
}

std::shared_ptr<Addition> operator+(
	const std::shared_ptr<Node>& lhs, const std::shared_ptr<Node>& rhs)
{
	return Addition::make(lhs, rhs);
}

/*****************************************************************************/

class Multiplication : public Node, public std::enable_shared_from_this<Multiplication>
{
public:
	static std::shared_ptr<Multiplication> make(std::shared_ptr<Node> lhs_, std::shared_ptr<Node> rhs_);

	double forward(const std::unordered_map<std::string, double>& env) override;
	std::vector<double> backward() override;
	std::vector<std::shared_ptr<Node>> children() override;

private:
	Multiplication(std::shared_ptr<Node> lhs_, std::shared_ptr<Node> rhs_);

	std::shared_ptr<Node> lhs;
	std::shared_ptr<Node> rhs;
};

Multiplication::Multiplication(std::shared_ptr<Node> lhs_, std::shared_ptr<Node> rhs_) : lhs(lhs_), rhs(rhs_) {}

std::shared_ptr<Multiplication> Multiplication::make(std::shared_ptr<Node> lhs_, std::shared_ptr<Node> rhs_)
{
	return std::shared_ptr<Multiplication>(new Multiplication(lhs_, rhs_));
}

double Multiplication::forward(const std::unordered_map<std::string, double>& env)
{
	// NOTE: evaluation order in a() + b() is undefined. This is why we use
	// temporary variables
	auto lhs_val = lhs->get_computed_value();
	auto rhs_val = rhs->get_computed_value();
	return lhs_val * rhs_val;
}

std::vector<double> Multiplication::backward() {
	return { this->rhs->get_computed_value(), this->lhs->get_computed_value() };
}

std::vector<std::shared_ptr<Node>> Multiplication::children()
{
	return { this->lhs, this->rhs };
}

std::shared_ptr<Multiplication> operator*(
	const std::shared_ptr<Node>& lhs, const std::shared_ptr<Node>& rhs)
{
	return Multiplication::make(lhs, rhs);
}

/*****************************************************************************/

// TODO(Andrea): use namespaces to avoid polluting the global namespace and
// fuck with includes
using env = std::unordered_map<std::string, double>;
using std::cout; using std::endl; using std::shared_ptr; using std::unordered_set; using std::vector; using std::reverse; using std::unordered_map;

void dag_post_order(shared_ptr<Node> node, unordered_set<int>& visited, vector<shared_ptr<Node>>& order) {
	visited.insert(node->get_id());
	for (auto& succ : node->children()) {
		if (visited.find(succ->get_id()) == visited.end()) dag_post_order(succ, visited, order);
	}
	order.push_back(node);
}

// TODO(Andrea): figure out how the API should look like. Probably forward should be private and exposed through a function
double eval(std::shared_ptr<Node> root, env eval_env) {
	unordered_set<int> visited;
	vector<std::shared_ptr<Node>> order;
	dag_post_order(root, visited, order);

	// TODO(Andrea): understand if I should use auto, auto&, auto&& wtf
	for (auto& n : order) {
		double val = n->forward(eval_env);
		n->set_computed_value(val);
		cout << "evaluation id=" << n->get_id() << " value=" << val << endl;
	}

	return root->get_computed_value();
}

void backprop(std::shared_ptr<Node> root) {
	vector<std::shared_ptr<Node>> order;

	unordered_set<int> visited;
	dag_post_order(root, visited, order);
	reverse(order.begin(), order.end());

	root->add_to_adjoint(1);
	for (auto& n : order) {
		for (int i = 0; i < n->children().size(); i++) {
			auto child = n->children()[i];
			auto adj = n->backward()[i];
			child->add_to_adjoint(adj * n->get_adjoint());
		}
	}
}

int main()
{
	// Create a new Node and a new shared pointer. Then put the node in the shared
	// pointer.
	auto two = Constant::make(2);
	auto three = Constant::make(3);
	auto four = Constant::make(4);

	auto add1 = two + three;
	auto mul1 = add1 * four;
	auto add2 = mul1 + three;

	env eval_env = { {"x1", -9.0} };

	cout << "eval should be equal to 23: " << eval(add2, eval_env) << endl;

	backprop(add2);
	cout << two->get_adjoint() << endl;
	cout << three->get_adjoint() << endl;
	cout << four->get_adjoint() << endl;
}

