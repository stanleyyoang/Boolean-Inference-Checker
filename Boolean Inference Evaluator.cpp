//Boolean Inference Evaluator Project
//By: Stanley Yoang

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <cmath>

using namespace std;

typedef struct AST *pNODE;
struct AST {
	string info;
	pNODE children[2];
};

struct tokRslt {
	bool success;
	vector<string> syms;
};

struct parseRslt {
	bool success;
	AST ast;
};

struct TPERslt {
	bool val;
	string msg;
};

struct inference {
	vector<AST> premises;
	AST conclusion;
};

//Function declarations
void prinTree(AST T);
pNODE cons(string s, pNODE c1, pNODE c2);
string ASTtoString(AST T);
tokRslt tokenize(string s);
parseRslt parse(vector<string> V, int start, int stop);
pNODE checkConstant(vector<string> V, int start, int stop);
pNODE checkUnbreakable(vector<string> V, int start, int stop);
pNODE checkNegation(vector<string> V, int start, int stop);
pNODE checkConjunction(vector<string> V, int start, int stop);
pNODE checkDisjunction(vector<string> V, int start, int stop);
pNODE checkImplication(vector<string> V, int start, int stop);
pNODE checkBooleanExpression(vector<string> V, int start, int stop);
bool eval(AST T);
list<string> vars(AST T);
void Insert(string s, list<string> *L);
list<string> mergedupe(list<string> l1, list<string> l2);
list<bool> bits(int i, int N);
pNODE copyAST(pNODE Exp);
AST substitute(list<bool> vals, list<string> vars, AST Exp);
void substituteHelp(list<bool> vals, list<string> vars, AST *Exp);
bool witnessInvalid(list<bool> vals, list<string> vars, inference I);
bool valid(inference I);
string validInference(string s);

void prinTree(AST T) {
	if (T.children[0] == NULL) {
		cout << T.info;
		return;
	}

	cout << "(" << T.info << " ";
	prinTree(*(T.children[0]));
	cout << " ";

	if (T.children[1] != NULL) {
		prinTree(*(T.children[1]));
	}

	cout << ")";
}

pNODE cons(string s, pNODE c1, pNODE c2) {
	pNODE ret = new AST;
	ret->info = s;  // same as (*ret).info = s
	ret->children[0] = c1;
	ret->children[1] = c2;
	return ret;
}

string ASTtoString(AST T) //converts an AST to String
{
	string s;
	// If both children are NULL, just print the symbol
	if (T.children[0] == NULL) {
		s = s + T.info;
		return s;
	}

	// print an opening paren, followed by the symbol of T, followed
	// by a space, and then the first child.
	s = s + "(";
	s = s + T.info;
	s = s + " ";
	s += ASTtoString(*(T.children[0]));
	s = s + " ";

	// print the second child if there is one, and then a right paren.
	if (T.children[1] != NULL) {
		s += ASTtoString(*(T.children[1]));
	}
	s = s + ")";
	return s;
}

tokRslt tokenize(string s)
{
	tokRslt tokenize;

	//Looks for correct characters, if not appropriate set to false
	for (size_t i = 0; i < s.length(); i++)
	{
		if (s[i] == 'T' || s[i] == 'F' || s[i] == '^' || s[i] == 'v' || s[i] == '~' || s[i] == '<' || s[i] == '=' || s[i] == '>' ||
			s[i] == '(' || s[i] == ')' || s[i] == ' ' || s[i] == ',' || s[i] == ':' || s[i] == '.' || islower(s[i]) != 0)
		{
			tokenize.success = true;
			continue;
		}
		else
		{
			tokenize.success = false;
			break;
		}
	}

	if (tokenize.success != false)
	{
		tokenize.success = true;

		//adds the characters to the vector string
		for (size_t i = 0; i < s.length(); i++)
		{
			//ignore white space
			if (s[i] == ' ')
				continue;

			//add => as one string
			else if (s[i] == '=')
			{
				if (s[i + 1] == '>')
				{
					tokenize.syms.push_back(s.substr(i, 2));
				}
				else
				{
					tokenize.success = false;
					break;
				}
				i++;
			}

			//add <=> as one string
			else if (s[i] == '<')
			{
				if (s[i + 1] == '=')
				{
					if (s[i + 2] == '>')
					{
						tokenize.syms.push_back(s.substr(i, 3));
					}
					else
					{
						tokenize.success = false;
						break;
					}
				}
				else
				{
					tokenize.success = false;
					break;
				}
				i = i + 2;
			}

			else if (s[i] == ':')
			{
				if (s[i + 1] == '.')
				{
					tokenize.syms.push_back(s.substr(i, 2));
				}
				else
				{
					tokenize.success = false;
					break;
				}
				i++;
			}

			else if (s[i] == 'v')
			{
				tokenize.syms.push_back(s.substr(i, 1));
			}

			else if (islower(s[i]) != 0)
			{
				int counter = 0;
				for (size_t x = i; islower(s[x]) != 0; x++)
				{
					counter++;
				}

				tokenize.syms.push_back(s.substr(i, counter));

				if (counter != 1)
					i += counter - 1;
			}
			//add the other symbols one by one
			else
			{
				if (s[i] == '.')
				{
					tokenize.success = false;
					break;
				}
				else
					tokenize.syms.push_back(s.substr(i, 1));
			}
		}
	}

	//if symbols are inappropriate, clear the vector
	if (tokenize.success == false)
		tokenize.syms.clear();

	return tokenize;
}

inference wholeParse(vector<string> V)
{
	inference infer;
	vector<AST> premises;
	parseRslt temp;
	AST conclusion;
	vector<int> index = { 0 };

	int open = 0;
	int close = 0;
	int counter = 0;

	//if vector is empty set AST to null
	if (V.size() == 0)
	{
		return infer;
	}

	if (V[0] == "^" || V[0] == "v" || V[0] == "=>" || V[0] == "<=>")
	{
		return infer;
	}

	//ensures that there is an equal amount of open and closed parentheses
	for (size_t i = 0; i < V.size(); i++)
	{
		if (V[i] == "(")
			open++;
		if (V[i] == ")")
			close++;
	}

	if (open != close)
	{
		return infer;
	}

	for (size_t i = 0; i < V.size(); i++)
	{
		if (V[i] == ":.")
			counter++;
	}

	if (counter > 1 || counter == 0)
	{
		premises.clear();
		infer.premises = premises;
		return infer;
	}

	for (size_t i = 0; i < V.size(); i++)
	{
		if (V[i] == ":.")
		{
			for (size_t k = i; k < V.size(); k++)
			{
				if (V[k] == ",")
				{
					return infer;
				}
			}
		}
	}

	for (size_t i = 0; i < V.size(); i++)
	{
		if (V[i] == "," || V[i] == ":.")
			index.push_back(i);
	}

	if (index.empty() == true)
	{
		return infer;
	}

	else
	{
		for (size_t i = 0; i < index.size(); i++)
		{
			if (index[i] == 0)
			{
				temp = parse(V, 0, index[i + 1]);
				if (temp.success == true)
					premises.push_back(temp.ast);
				else
				{
					premises.clear();
					infer.premises = premises;
					return infer;
				}
			}

			else if (i == index.size() - 1)
			{
				temp = parse(V, index[i] + 1, V.size());
				if (temp.success == true)
					conclusion = temp.ast;
				else
				{
					premises.clear();
					infer.premises = premises;
					return infer;
				}
			}

			else
			{
				temp = parse(V, index[i] + 1, index[i + 1]);
				if (temp.success == true)
					premises.push_back(temp.ast);
				else
				{
					premises.clear();
					infer.premises = premises;
					return infer;
				}
			}
		}
	}
	infer.premises = premises;
	infer.conclusion = conclusion;

	return infer;
}


//Function used to put the vector string into an AST
//Also ensures that symbols are put in a correct order
parseRslt parse(vector<string> V, int start, int stop)
{
	parseRslt expression;
	pNODE expression2;

	if (V[start] == "^" || V[start] == "v" || V[start] == "=>" || V[start] == "<=>")
	{
		expression.success = false;
		return expression;
	}

	//create the AST
	expression2 = checkBooleanExpression(V, start, stop);

	if (expression2 == NULL)
	{
		expression.success = false;
		return expression;
	}
	else
	{
		expression.success = true;
		expression.ast = *expression2;
		return expression;
	}
}

pNODE checkConstant(vector<string> V, int start, int stop)
{
	if (start != stop - 1 || start >= (int)V.size())
	{
		return NULL;
	}

	if (V[start] == "T" || V[start] == "F" || islower(V[start][0]) != 0)
	{
		return cons(V[start], NULL, NULL);
	}
	else
	{
		return cons("EMPTY", NULL, NULL);
	}
}

pNODE checkUnbreakable(vector<string> V, int start, int stop)
{
	pNODE expression;

	if (V[start] == "(" && V[stop - 1] == ")")
	{
		expression = checkBooleanExpression(V, start + 1, stop - 1);
		if (expression != NULL)
			return expression;
	}

	expression = checkConstant(V, start, stop);
	if (expression != NULL)
		return expression;
	else
		return NULL;
}

pNODE checkNegation(vector<string> V, int start, int stop)
{
	pNODE expression;

	for (int i = start; i < stop; i++)
	{
		if (V[i] == "~")
		{
			expression = checkNegation(V, start + 1, stop);
			if (expression != NULL)
				return cons(V[i], expression, NULL);
		}
	}

	expression = checkUnbreakable(V, start, stop);
	if (expression != NULL)
		return expression;
	else
		return NULL;
}

pNODE checkConjunction(vector<string> V, int start, int stop)
{
	pNODE expression1, expression2;

	for (int i = start + 1; i < stop; i++)
	{
		if (V[i] == "^")
		{
			expression1 = checkConjunction(V, start, i);
			expression2 = checkNegation(V, i + 1, stop);
			if (expression1 != NULL && expression2 != NULL)
				return cons(V[i], expression1, expression2);
		}
	}

	expression1 = checkNegation(V, start, stop);

	if (expression1 != NULL)
		return expression1;
	else
		return NULL;
}

pNODE checkDisjunction(vector<string> V, int start, int stop)
{
	pNODE expression1, expression2;

	for (int i = start + 1; i < stop; i++)
	{
		if (V[i] == "v")
		{
			expression1 = checkDisjunction(V, start, i);
			expression2 = checkConjunction(V, i + 1, stop);
			if (expression1 != NULL && expression2 != NULL)
				return cons(V[i], expression1, expression2);
		}
	}

	expression1 = checkConjunction(V, start, stop);

	if (expression1 != NULL)
		return expression1;
	else
		return NULL;
}

pNODE checkImplication(vector<string> V, int start, int stop)
{
	pNODE expression1, expression2;

	for (int i = start + 1; i < stop; i++)
	{
		if (V[i] == "=>")
		{
			expression1 = checkDisjunction(V, start, i);
			expression2 = checkImplication(V, i + 1, stop);
			if (expression1 != NULL && expression2 != NULL)
				return cons(V[i], expression1, expression2);
		}
	}

	expression1 = checkDisjunction(V, start, stop);

	if (expression1 != NULL)
		return expression1;
	else
		return NULL;
}

pNODE checkBooleanExpression(vector<string> V, int start, int stop)
{
	pNODE expression1, expression2;

	for (int i = start + 1; i < stop; i++)
	{
		if (V[i] == "<=>")
		{
			expression1 = checkImplication(V, start, i);
			expression2 = checkBooleanExpression(V, i + 1, stop);
			if (expression1 != NULL && expression2 != NULL)
				return cons(V[i], expression1, expression2);
		}
	}

	expression1 = checkImplication(V, start, stop);

	if (expression1 != NULL)
		return expression1;
	else
		return NULL;
}

bool eval(AST T)
{
	bool left, right;

	//if AST doesn't have any children, it must be a T or F value
	if (T.children[0] == NULL)
	{
		if (T.info == "T")
			return true;
		else if (T.info == "F")
			return false;
		else
			return NULL;
	}

	//if AST.info is "~" and only has one child invert the value
	else if (T.info == "~" && T.children[1] == NULL)
	{
		return !eval(*T.children[0]);
	}

	//follow the standard semantics of boolean expressions
	else
	{
		//AND statements
		if (T.info == "^")
		{
			left = eval(*T.children[0]);
			right = eval(*T.children[1]);

			if (left == true && right == true)
				return true;
			else
				return false;
		}

		//OR statements
		else if (T.info == "v")
		{
			left = eval(*T.children[0]);
			right = eval(*T.children[1]);

			if (left == false && right == false)
				return false;
			else
				return true;
		}

		//IMPLIES statements
		else if (T.info == "=>")
		{
			left = eval(*T.children[0]);
			right = eval(*T.children[1]);

			if (left == true && right == false)
				return false;
			else
				return true;
		}

		//IFF statements
		else if (T.info == "<=>")
		{
			left = eval(*T.children[0]);
			right = eval(*T.children[1]);

			if (left == right)
				return true;
			else
				return false;
		}
		return NULL;
	}
}

list<string> vars(AST T)
{
	list<string> var, left, right;

	if (T.children[0] == NULL)
	{
		if (T.info == "T" || T.info == "F")
			return {};
		else
		{
			Insert(T.info, &var);
			return var;
		}
	}
	else if (T.info == "~" && T.children[1] == NULL)
	{
		return vars(*(T.children[0]));
	}
	else
	{
		left = vars(*(T.children[0]));
		right = vars(*(T.children[1]));
		return mergedupe(left, right);
	}
}

void Insert(string s, list<string> *L)
{
	list<string> temp = *L;
	temp.unique();

	if (is_sorted(L->begin(), L->end()) && *L == temp)
	{
		if (find(L->begin(), L->end(), s) != L->end())
			return;
		else
		{
			L->push_back(s);
			L->sort();
			L->unique();
		}
	}
}

list<string> mergedupe(list<string> l1, list<string> l2)
{
	list<string> listfinal;

	listfinal.merge(l1);
	listfinal.merge(l2);

	listfinal.unique();

	return listfinal;
}

list<string> vars(vector<AST> Ts)
{
	list<string> temp;
	list<string> listfinal;

	for (size_t i = 0; i < Ts.size(); i++)
	{
		temp = vars(Ts[i]);
		listfinal.merge(temp);
	}

	listfinal.unique();

	return listfinal;
}

list <bool> bits(int i, int N)
{
	list <bool> bit;
	bool remainder;

	if (N > 0 && (i >= 0 && i <= pow(2, N) - 1))
	{
		while (i > 0)
		{
			remainder = i % 2;
			bit.push_front(remainder);
			i /= 2;
		}
	}
	else
	{
		bit.clear();
		return bit;
	}

	if (bit.size() != N)
	{
		for (int x = bit.size(); x < N; x++)
		{
			bit.push_front(0);
		}
	}
	
	return bit;
}

pNODE copyAST(pNODE Exp)
{
	if (Exp->children[0] == NULL)
		return cons(Exp->info, NULL, NULL);
	else if (Exp->info == "~")
		return cons(Exp->info, copyAST(Exp->children[0]), NULL);
	else
		return cons(Exp->info, copyAST(Exp->children[0]), copyAST(Exp->children[1]));
}

AST substitute(list<bool> vals, list<string> vars, AST Exp)
{
	AST temp = *copyAST(&Exp);
	substituteHelp(vals, vars, &temp);
	return temp;
}

void substituteHelp(list<bool> vals, list<string> vars, AST *help)
{
	vector<bool> val{ begin(vals), end(vals) };
	vector<string> var{ begin(vars), end(vars) };
	AST left, right;

	if (var.size() == val.size())
	{
		if (help->children[0] == NULL)
		{
			for (size_t i = 0; i < var.size(); i++)
			{
				if (var[i] == help->info)
				{
					if (val[i] == 0) {
						help->info = "F";
					}
					else if (val[i] == 1) {
						help->info = "T";
					}
				}
			}
		}
		else if (help->info == "~" && help->children[1] == NULL)
		{
			return substituteHelp(vals, vars, help->children[0]);
		}
		else
		{
			substituteHelp(vals, vars, help->children[0]);
			substituteHelp(vals, vars, help->children[1]);
		}
	}
}

bool witnessInvalid(list<bool> vals, list<string> vars, inference I)
{
	bool temp;


	if (eval(substitute(vals, vars, I.conclusion)) == false)
	{
		for (size_t i = 0; i < I.premises.size(); i++)
		{
			if (eval(substitute(vals, vars, I.premises[i])) != false)
			{
				temp = true;
			}
			else
			{
				temp = false;
				break;
			}
		}
		if (temp == true)
			return true;
		else
			return false;
	}
	else
		return false;
}

bool valid(inference I)
{
	bool temp;
	vector<AST> ASTs = I.premises;
	ASTs.push_back(I.conclusion);

	list<string> Lvar = vars(ASTs);
	
	int N = Lvar.size();
	int power = pow(2, N) - 1;

	for (int i = 0; i <= power; i++)
	{
		temp = witnessInvalid(bits(i, N), Lvar, I);

		if (temp == true) {
			return false;
		}
	}

	return true;
}

string validInference(string s)
{
	tokRslt token = tokenize(s);

	if (token.success == false)
		return "symbol error";

	inference infer = wholeParse(token.syms);

	if (infer.premises.size() == 0)
		return "grammar error";

	//check for valid or invalid inference
	if (valid(infer) == true)
		return "valid";
	else
		return "invalid";
}

int main()
{
	cout << validInference("a,,:...");
	return 0;
}