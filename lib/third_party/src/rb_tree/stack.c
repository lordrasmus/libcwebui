/*

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that neither the name of Emin
Martinian nor the names of any contributors are be used to endorse or
promote products derived from this software without specific prior
written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "WebserverConfig.h"


#ifdef USE_EXTERNAL_RBTREE
	//#warning "using external RB Tree"
#else

#include "stack.h"

int StackNotEmpty(stk_stack * theStack) {
	if ( theStack == 0 )
		return 0;
	if ( theStack->top == 0 )
		return 0;
  return 1;
}

/*int StackNotEmpty(stk_stack * theStack) {
 return (int) ( theStack ? (int) theStack->top : 0);
 }*/

stk_stack * StackJoin(stk_stack * stack1, stk_stack * stack2) {
	if (!stack1->tail) {
		free(stack1);
		return (stack2);
	} else {
		stack1->tail->next = stack2->top;
		stack1->tail = stack2->tail;
		free(stack2);
		return (stack1);
	}
}

stk_stack * StackCreate( void ) {
	stk_stack * newStack;

	newStack = (stk_stack *) SafeMalloc(sizeof(stk_stack));
	newStack->top = newStack->tail = NULL;
	return (newStack);
}

void StackPush(stk_stack * theStack, DATA_TYPE newInfoPointer) {
	stk_stack_node * newNode;

	if(!theStack->top) {
		newNode=(stk_stack_node *) SafeMalloc(sizeof(stk_stack_node));
		newNode->info=newInfoPointer;
		newNode->next=theStack->top;
		theStack->top=newNode;
		theStack->tail=newNode;
	} else {
		newNode=(stk_stack_node *) SafeMalloc(sizeof(stk_stack_node));
		newNode->info=newInfoPointer;
		newNode->next=theStack->top;
		theStack->top=newNode;
	}

}

DATA_TYPE StackPop(stk_stack * theStack) {
	DATA_TYPE popInfo;
	stk_stack_node * oldNode;

	if(theStack->top) {
		popInfo=theStack->top->info;
		oldNode=theStack->top;
		theStack->top=theStack->top->next;
		free(oldNode);
		if (!theStack->top) theStack->tail=NULL;
	} else {
		popInfo=NULL;
	}
	return(popInfo);
}

#if 0
static void StackDestroy(stk_stack * theStack, void DestFunc(void * a)) {

	stk_stack_node * y;

	if (theStack) {
		stk_stack_node * x = theStack->top;
		while (x) {
			y = x->next;
			DestFunc(x->info);
			free(x);
			x = y;
		}
		free(theStack);
	}
}
#endif

#endif /* USE_EXTERNAL_RBTREE */
