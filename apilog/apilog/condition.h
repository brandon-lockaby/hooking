#include <string>
using namespace std;

struct ConditionContext
{

};

class Condition
{
//private:
public:
	enum EType
	{
		Imm,
		Reg,
		Cond
	};

	EType mFirstType;
	EType mSecondType;

	// values
	void* mFirst;
	void* mSecond;

	// number of times to dereference values
	int mFirstDerefs;
	int mSecondDerefs;

	enum EOperation
	{
		Equal
	};

	EOperation mOperation;
private:
	static void* resolve(EType type, void* value, int derefs, ConditionContext context);

public:
	Condition(string input);
	Condition(EType first_type, void* first, int first_derefs, EType second_type, void* second, int second_derefs, EOperation operation);
	void* evaluate(ConditionContext context);
};
