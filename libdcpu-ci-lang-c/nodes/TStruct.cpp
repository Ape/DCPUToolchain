
#include "TStruct.h"
#include "NDeclarations.h"
#include "Lists.h"
#include <cmath>
	
		
void TStruct::resolveStruct()
{
	if (this->m_resolvedStruct != NULL)
		return;
		
	if (this->m_context == NULL)
		throw new CompilerException(0, "<internal>", 
		"Context for struct not initialized (internal error).");

	// Search AST for struct nodes.
	NDeclarations* decls = (NDeclarations*)m_context->m_RootNode;

	for (DeclarationList::iterator i = decls->definitions.begin(); i != decls->definitions.end(); i++)
		if ((*i)->cType == "statement-declaration-structure")
			if (((NStructureDeclaration*)*i)->id.name == this->m_name)
			{
				this->m_resolvedStruct = ((NStructureDeclaration*)*i);
				return;
			}

	throw new CompilerException(0, "<internal>", "Unknown struct type " + this->m_name + " encountered!");
	// TODO throw type exception, which is catched outside the type-system
	//       from where it is rethrown including line and file information
}

void TStruct::initContext(AsmGenerator& context)
{
	this->m_context = &context;
}

size_t TStruct::getBitSize()
{
	this->resolveStruct();
	return this->m_resolvedStruct->getBitSize(*m_context);
}

uint16_t TStruct::getWordSize()
{
	return (int)std::ceil((double)this->getBitSize() / 16.0);
}

uint16_t TStruct::getWordSize(AsmGenerator& context)
{
	return this->getWordSize();
}

uint16_t TStruct::getStructFieldPosition(std::string name)
{
	// Resolve struct if not already done.
	this->resolveStruct();

	// Count up the position.
	size_t pos = 0;

	for (VariableList::iterator i = this->m_resolvedStruct->fields.begin(); i != this->m_resolvedStruct->fields.end(); i++)
	{
		if ((*i)->id.name == this->m_name)
			return pos;
		else
			pos += (*i)->type.getWordSize(*m_context);
	}

	// If the field wasn't found...
	throw new CompilerException(0, "<internal>", "Unable to lookup field " + name + " in structure " + this->m_resolvedStruct->id.name + "!");
	// TODO throw type exception, which is catched outside the type-system
	//       from where it is rethrown including line and file information
}
		
/* copy */

// indirect copy given references (copies values)
AsmBlock* TStruct::copyByRef(char fromRef, char toRef)
{
	AsmBlock* block = new AsmBlock();
	for (unsigned int pos = 0; pos < this->getWordSize(); ++pos)
	{
		*block <<	"	SET [" << toRef << "], [" << fromRef << "]" << std::endl;
		*block <<	"	ADD " << toRef << ", 1" << std::endl;
		*block <<	"	ADD " << fromRef << ", 1" << std::endl;
	}
	*block <<	"	SUB " << toRef << ", " << this->getWordSize() << std::endl;
	*block <<	"	SUB " << fromRef << ", " << this->getWordSize() << std::endl;
	return block;
}

/* because all structs are referenced and there are no direct values, */
/* all the following functions map to copyByRef */

// direct copy via registers
AsmBlock* TStruct::copyValue(char from, char to)
{
	return copyByRef(from, to);
}
// saves value in "from" register into the reference
AsmBlock* TStruct::saveToRef(char from, char toRef)
{
	return copyByRef(from, toRef);
}
// load from a reference into a value
AsmBlock* TStruct::loadFromRef(char fromRef, char to)
{
	return copyByRef(fromRef, to);
}

/*************/
/* stack ops */
/*************/

AsmBlock* TStruct::pushStack(char a) {
	AsmBlock* block = new AsmBlock();
	*block <<	"	SUB SP, " << this->getWordSize() << std::endl;
	for (unsigned int pos = 0; pos < this->getWordSize(); ++pos)
	{
		*block <<	"	SET PEEK, [" << a << "]" << std::endl;
		*block <<	"	ADD SP, 1" << std::endl;
		*block <<	"	ADD " << a << ", 1" << std::endl;
	}
	*block <<	"	SUB SP, " << this->getWordSize() << std::endl;
	*block <<	"	SUB " << a << ", " << this->getWordSize() << std::endl;
	return block;
}


// FIXME do i need this? virtual AsmBlock* pushStackByRef(char a);


AsmBlock* TStruct::popStack()
{
	AsmBlock* block = new AsmBlock();
	*block <<	"	ADD SP, " << this->getWordSize() << std::endl;
	return block;
}

AsmBlock* TStruct::popStackClean()
{
	AsmBlock* block = new AsmBlock();
	for (unsigned int pos = 0; pos < this->getWordSize(); ++pos)
	{
		*block <<	"	SET PEEK, 0" << std::endl;
		*block <<	"	ADD SP, 1" << std::endl;
	}
	return block;
}




