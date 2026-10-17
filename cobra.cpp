#include <iostream>
#include <unordered_map>
#include <stack>
#include <cmath>
#include <string>
#include <cstdlib>
#include <cctype>
#include <fstream>
#include <filesystem>

class Acc {
	double val;
	bool state;

public:
	Acc() : val{}, state{true} {}

	double pop() {
		state = true;
		return val;
	}

	void push(double val) {
		this->val = val;
		state = false;
	}

	bool empty() {
		return state;
	}
};

class Op {
public:
	enum Type {
		ADD,
		SUB,
		MUL,
		DIV,
		POW,
		SQRT,
		PRINT,
		INPUT,
		MIN,
		MAX,
		INIT,
	TYPES};
private:

	static const std::unordered_map<std::string, Op> ops;

	double acc;
	short cost, depth;
	Type type;

	Op(double _acc, short _cost, Type _type) : acc{_acc}, cost{_cost}, depth{}, type{_type} {}

public:
	Op() = default;

	static bool init(const char *rep, int depth, Op &op) {
		auto it = ops.find(rep);
		if(it == ops.end())
			return false;
		op = it->second;
		op.cost += depth * TYPES;
		op.depth = depth;
		return true;
	}

	Type getType() {
		return type;
	}

	void setAcc(double val) {
		acc = val;
	}

	short getCost() {
		return cost;
	}

	short getDepth() {
		return depth;
	}

	void invoke(Acc &ext) {
		switch(type) {
			case ADD: ext.push(acc + ext.pop()); break;
			case SUB: ext.push(acc - ext.pop()); break;
			case MUL: ext.push(acc * ext.pop()); break;
			case DIV: ext.push(acc / ext.pop()); break;
			case POW: ext.push(std::pow(acc, ext.pop())); break;
			case SQRT: ext.push(std::sqrt(ext.pop())); break;
			case PRINT: std::cout << "<<< " << ext.pop() << std::endl; break;
			case INPUT: {
				std::cout << ">>> ";
				std::cin >> acc;
				ext.push(acc);
			} break;
			case MIN: {
				double val = ext.pop();
				ext.push((acc = acc < val ? acc : val));
			} break;
			case MAX: {
				double val = ext.pop();
				ext.push((acc = acc > val ? acc : val));
			} break;
			case INIT: {
				double val = ext.pop();
				union {
					double val;
					double *ptr;
				} var;
				var.val = acc;
				*var.ptr = val;
			} break;
		}
	}
};

const std::unordered_map<std::string, Op> Op::ops = {
	{"+", {0, 1, ADD}},
	{"-", {0, 1, SUB}},
	{"*", {0, 2, MUL}},
	{"/", {0, 2, DIV}},
	{"^", {0, 4, POW}},
	{"sqrt", {0, 3, SQRT}},
	{"print", {NAN, 3, PRINT}},
	{"input", {0, 3, INPUT}},
	{"min", {std::numeric_limits<double>::infinity(), 3, MIN}},
	{"max", {-std::numeric_limits<double>::infinity(), 3, MAX}},
	{"=", {0, 0, INIT}}
};

void invoke(char *exp) {
	#define unwind(cond) while(!ops.empty() && ops.top().getDepth() >= depth && cond) {\
			ops.top().invoke(acc);\
			ops.pop();\
		}

	std::unordered_map<std::string, double> vars;
	Acc acc;
	std::stack<Op> ops;
	Op op;
	short depth = 0;
	union {
		double val;
		double *ptr;
	} var;
	for(char *c = exp; *c; ++c) {
		if(isspace(*c))
			continue;
		if(*c == ';') {
			if(depth) throw "Syntax error: Mismatched parentheses";
			unwind(true);
		} else if(*c == '(')
			++depth;
		else if(*c == ')')
			--depth;
		else if(*c == ',') {
			unwind(true);
			ops.top().invoke(acc);
		} else if(isdigit(*c) || *c == '.') {
			acc.push(atof(c));
			while(isdigit(*(c+1)) || *(c+1) == '.') ++c;
		} else {
			char *p = c;
			if(ispunct(*c)) while(ispunct(*p) && *p != '.' && *p != '(' && *p != ')' && *p != ',') ++p;
			else while(*p && !isspace(*p) && !ispunct(*p)) ++p;
			char temp = *p;
			*p = 0;
			if(!Op::init(c, depth, op)) {
				var.ptr = &vars[c];
				acc.push(*var.ptr);
				*p = temp;
				c = p - 1;
				continue;
			}
			*p = temp;
			if(!ops.empty() && op.getCost() <= ops.top().getCost())
				unwind(op.getCost() < ops.top().getCost());
			if(op.getType() == Op::INIT)
				op.setAcc(var.val);
			else if(!acc.empty())
				op.setAcc(acc.pop());
			ops.push(op);
			c = p - 1;
		}
	}
}

int main(int argc, char **argv) {
	if(argc < 2) {
		std::cerr << "Missing argument!" << std::endl;
		return 1;
	}
	if(argc > 2) {
		std::cerr << "Too many arguments!" << std::endl;
		return 1;
	}
	if(!std::filesystem::exists(argv[1])) {
		std::cerr << "File does not exist!" << std::endl;
		return 1;
	}
	auto size = std::filesystem::file_size(argv[1]);
	std::string code(size + 1, '\0');
	std::ifstream file(argv[1]);
	file.read(code.data(), size);
	try {
		invoke(code.data());
	} catch(std::string msg) {
		std::cerr << msg << std::endl;
	}
	return 0;
}
