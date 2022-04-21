//-----------------------------------------------------------------------------
//
// Torque Script Array Object
//
// by Emil Diego
// ediego@miami.edu
//
// Since TorqueScript doesn't have an actual array implementation I created this class
// so I could create arrays and passs them as function parameters.
// I use the STL Vector to implement the array class and support simple
// access to the array elements.
//
// Methods
// add(xValue)					: Add a value to the end of the array
// echo()						: Prints out the contents of the array to the console
// getValue(iIndex)				: Retrevies the value at the specified index
// setValue(iIndex, xValue)		: Sets the value at the specified index
// clear()						: Clears the contents of the array
// findValue(xValue)			: Finds the first occurance of xValue and returns the index
// length()						: Returns the number of elements in the array
//
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/simBase.h"
#include "console/consoleTypes.h"
#include "math/mMathFn.h"
#include "core/stringTable.h"

#include	<stdlib.h>
#include	<stdio.h>

class Array : public SimObject
{
   typedef SimObject Parent;

private:
	struct Element
	{
		StringTableEntry value;
	};


public:

	Vector<Element> m_array;

	Array();
	bool onAdd();
	void onRemove();

	// Public methods
	void					add(StringTableEntry xValue);	
	void					echo();

	StringTableEntry		getValue(U32 iIndex);
	void					setValue(U32 iIndex, StringTableEntry xValue);

	void					clear();

	S32						findValue(StringTableEntry xValue);
	S32						length();
	

	DECLARE_CONOBJECT(Array);


};

IMPLEMENT_CONOBJECT(Array);

//-----------------------------------------------------------------------------
// CONSTRUCTOR
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Array
//
// Default constructor for the class.
//-----------------------------------------------------------------------------
Array::Array()
{
	m_array.clear();
}


//-----------------------------------------------------------------------------
// onAdd onRemove
//-----------------------------------------------------------------------------
bool Array::onAdd()
{
   if(!Parent::onAdd())
      return false;

   return true;
}

void Array::onRemove()
{
	m_array.empty();
	Parent::onRemove();
}


//-----------------------------------------------------------------------------
// PUBLIC METHODS
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// length
//
// Return the number of elements in the array
//-----------------------------------------------------------------------------
S32 Array::length()
{
	return this->m_array.size();
}

//-----------------------------------------------------------------------------
// echo
//
// Displays all the values stored in the array to the console
//-----------------------------------------------------------------------------
void Array::echo()
{
	Con::printf("Array Values:");
	Con::printf("Index	Value");
	for(U32 i = 0; i < this->m_array.size(); i++)
	{
		StringTableEntry xTmpVal = this->m_array[i].value;

		Con::printf("%d	- %s",(S32)i, xTmpVal);
	}
}

//-----------------------------------------------------------------------------
// add
//
// Adds a value to the end of the array.
//-----------------------------------------------------------------------------
// PARAMETERS
// value	: The value we want to add to the array
//-----------------------------------------------------------------------------
void Array::add(StringTableEntry xValue)
{
	Element		xTmpNode;

	// Set the value of the node we want to store in the vector
	xTmpNode.value			= xValue;

	// Add the node to the end of the vector
	this->m_array.push_back(xTmpNode);

}

//-----------------------------------------------------------------------------
// getValue
//
// Return the value stored at the specified index.  IF the index is out 
// of range, then return null.
//-----------------------------------------------------------------------------
// PARAMETERS
// iIndex		: The index of the array element we want to retreive
//-----------------------------------------------------------------------------
StringTableEntry Array::getValue(U32 iIndex)
{
	if (iIndex > this->m_array.size() || iIndex < 0)
		return NULL;

	return this->m_array[iIndex].value;

}

//-----------------------------------------------------------------------------
// setValue
//
// Sets the value of the specified array element.  If the index is out
// of range thendo nothing.
//-----------------------------------------------------------------------------
// PARAMETERS
// iIndex		: The index of the element we want to set
// xValue		: The value we want to set the array element to
//-----------------------------------------------------------------------------
void Array::setValue(U32 iIndex, StringTableEntry xValue)
{
	if (iIndex > this->m_array.size() || iIndex < 0)
		return;

	this->m_array[iIndex].value = xValue;

}

//-----------------------------------------------------------------------------
// clear
//
// Remove all the entries from the array
//-----------------------------------------------------------------------------
void Array::clear()
{
	this->m_array.clear();
}

//-----------------------------------------------------------------------------
// findValue
//
// Searches the aray for the specified value and returns the index.  If the 
// value is not in the array, then return -1.
//-----------------------------------------------------------------------------
S32 Array::findValue(StringTableEntry xValue)
{
	StringTableEntry	xTmpEntry;

	for (U32 i = 0; i < this->m_array.size(); i++)
	{		
		// Get the current entry in the array.
		xTmpEntry = this->m_array[i].value;

		// Now compare it to the parameter to see if they are ==
		S32 result = dStricmp(xTmpEntry, xValue);
		if (result == 0)
			return i;
	}
	// The value was not found in the array
	return -1;
}

//-----------------------------------------------------------------------------
// CONSOLE FUNCTIONS
//-----------------------------------------------------------------------------
ConsoleMethod( Array, length, S32, 2, 2, "Returns the number of elements in the array.")
{
	return object->length();
}

ConsoleMethod( Array, getValue, const char *, 3, 3, "(U32 iIndex)"
              "Return the value stored at the specified array index.")
{
   U32 iTmpIndex = atoi(argv[2]);
   return object->getValue(iTmpIndex); 
}

ConsoleMethod( Array, setValue, void, 4, 4, "(U32 iIndex, StringTableEntry xValue"
				"Sets the specified element of the array to xValue.")
{

	U32							iTmpIndex = 0;
	StringTableEntry			xTmpValue;

	// Get the parameters
	iTmpIndex = atoi(argv[2]);
	xTmpValue = StringTable->insert(argv[3]);

	// Now set the array element
	object->setValue(iTmpIndex, xTmpValue);
	return;

}

ConsoleMethod( Array, echo, void, 2, 2, "Prints all the values of the array to the console.")
{

	object->echo();
}

ConsoleMethod( Array, add, void, 3, 3, "(StringTableEntry xValue)"
			  "Adds a value to the end of the array.")
{
	StringTableEntry		xTmpValue;

	// Get the value we want to add
	xTmpValue = StringTable->insert(argv[2]);

	// Add the value to the array
	object->add(xTmpValue);

}

ConsoleMethod( Array, clear, void, 2, 2, "Clears all the entries from the array.")
{
	object->clear();
}

ConsoleMethod( Array, findValue, S32, 3, 3, "(StringTableEntry xValue)"
			  "Searches the array for the first occurence of the value specified.  If the value is found then return the index, otherwise return -1")
{
	StringTableEntry		xTmpValue;
	S32						iTmpIndex = 0;
	char					sBuffer[64];

	// Get the value
	xTmpValue = StringTable->insert(argv[2]);

	// Find the value in the array and return the result
	return object->findValue(xTmpValue);

	

}