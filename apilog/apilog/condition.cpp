#include "condition.h"

Condition::Condition(EType first_type, void* first, int first_derefs, EType second_type, void* second, int second_derefs, EOperation operation)
:	mFirstType(first_type),
	mFirst(first),
	mFirstDerefs(first_derefs),
	mSecondType(second_type),
	mSecond(second),
	mSecondDerefs(second_derefs),
	mOperation(operation)
{
}

Condition::Condition(string input)
{

}

void* Condition::resolve(EType type, void* value, int derefs, ConditionContext context)
{
	switch(type)
	{
	case Cond:
		value = ((Condition*)value)->evaluate(context);
	case Imm:
	default:
		for(int i = 0; i < derefs; i++)
		{
			value = (void*)*(unsigned int*)value;
		}
		break;
	}
	return value;
}

void* Condition::evaluate(ConditionContext context)
{
	void* first = resolve(mFirstType, mFirst, mFirstDerefs, context);
	void* second = resolve(mSecondType, mSecond, mSecondDerefs, context);

	// operate
	void* result = 0;
	switch(mOperation)
	{
	case Equal:
	default:
		result = (void*)(first == second);
		break;
	}

	return result;
}
