/*
*********************************************************************************************************
*                                      SCHEDULING BINOMIAL HEAP
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*                        An implementation of top-down splaying
*                            D. Sleator <sleator@cs.cmu.edu>
*                                   March 1992
*********************************************************************************************************
*/
/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include<stdio.h>
#include<os.h>

/*MEMORY PARTITION FOR HEAP*/
CPU_INT32U *MemoryPartition_heap [1][15];               //we need only one heap.
OS_MEM MemoryCB_heap;

/*memory partition for nodes inside the heap*/
CPU_INT32U *MemoryPartition_heap_node [10][10];
OS_MEM MemoryCB_heap_node;

void heap_insert(NODE*);
NODE* merge(NODE*, NODE*);

HEAP* HEAP1; 
/*
*********************************************************************************************************
*              BINOMIAL HEAP INITILIZATION - CREATION OF HEAP AND NODES INSIDE THE HEAP
*
* Description : This function is called by TASK MANAGER / APPSTARTTASK 
*
* Arguments   : none
*
* Returns     : none
*
* Notes       : one time initialization
*               
*********************************************************************************************************
*/


//Initialize the heap pointer first before adding the first node
void heap_init()
{ 
  OS_ERR      err, err1;
  OS_MEM_QTY node_size = sizeof(CPU_INT32U);
  OSMemCreate((OS_MEM*)&MemoryCB_heap, (CPU_CHAR*)"binomial_heap", &MemoryPartition_heap[0][0], (OS_MEM_QTY)(10), (OS_MEM_SIZE)(10*node_size), &err);
  HEAP1=NULL;
  OSMemCreate((OS_MEM*)&MemoryCB_heap_node, (CPU_CHAR*)"binomial_heap_node", &MemoryPartition_heap_node[0][0], (OS_MEM_QTY)(10), (OS_MEM_SIZE)(10*node_size), &err1);
}
    
void heap_create()
{
    heap_init();
    OS_ERR  err;
    HEAP1=(HEAP*)OSMemGet((OS_MEM*)&MemoryCB_heap, (OS_ERR*)&err);
   
     if (HEAP1 == NULL)                         //it means memory not allocated
      err = OS_ERR_Z;
     else
     {
        HEAP1->min  = NULL;
        HEAP1->entries=0;
        for(int i=0;i<size_of_array;i++)
        {
        HEAP1->array[i]=NULL;
        }  
      
      } 
}

/*
***********************************************************************************************************************************
*                                  FIND MINIMUM AND RETURN POINTER TO THE MINIMUM
*
* Description : Splay top-down approach
*
* Arguments   : none
*
* Returns     : none
*
* Notes       : Application should not use this function , update the total number of tasks used in os.h
*               
**************************************************************************************************************************************
*/
/************************************POINTER TO THE MINIMUM IS ALWAYS MAINTAINED***********************************************************************************/
NODE* find_min()
{
    HEAP1->min = HEAP1->array[0];
    if(HEAP1->array[1]!=NULL)
    {
    for(int i=1;i<HEAP1->entries;i++)
    {
        if(HEAP1->array[i]->value <  HEAP1->min->value)
        HEAP1->min=HEAP1->array[i];
    }
    }
    return HEAP1->min;
}

/********************CREATE POINTERS TO THE ROOT OF EVERY BINARY TREE IN THE BINOMIAL HEAP***********************************************************************************/
void create_tree_roots(NODE* new_node)
{
  OS_ERR err;
    NODE* roots = (NODE*)OSMemGet((OS_MEM*)&MemoryCB_heap, (OS_ERR*)&err);
    roots=new_node;
    HEAP1->array[HEAP1->entries]=roots;
    HEAP1->entries++;
}

/**********************INITIALIZE THE NODES AFTER CREATION AND INSERT THEM INTO THE HEAP************/
void heap_node_create(OS_TCB* ptcb, OS_TASK_DEADLINE deadline_value)
{
  OS_ERR err1;
    NODE* new_node=(NODE*)OSMemGet((OS_MEM*)&MemoryCB_heap_node, (OS_ERR*)&err1);
    new_node->next   = NULL;
    new_node->child  = NULL;
    new_node->degree = 0;          //when node is created, degree is zero.
    new_node->value  = deadline_value;
    new_node->ptcb=ptcb;
    
    //Create pointer to the root of the tree, insert it into the array and increase the enries in the array;
    create_tree_roots(new_node);
    
    if(HEAP1->min==NULL)
    {
        HEAP1->min=new_node;            //At the beginning, min will always point to the first node created.
    }
    else
        {
        heap_insert(new_node);
        }
     find_min();                        //everytime a new element is inserted, check min and refresh it
    
}

/****************************************** INSERTING NODE INTO THE HEAP***************************/
void heap_insert(NODE* node)
{
    int i=0;
    //start comparing from the first root pointer in the array until you find no root element in the array
    while( HEAP1->array[i]!=NULL)
    {
        if(node->degree==HEAP1->array[i]->degree)
           {
               if(HEAP1->array[i] != node)
               {
                   NODE* merged_tree = merge(node, (HEAP1->array[i])); //call merge
                   heap_insert(merged_tree);
               }   
           }
            i=i+1;
    }
}

/****************************************** MERGING OF THE TREES WITH SAME DEGREE ***************************/
NODE* merge(NODE* master, NODE* child)
{
  OS_ERR err;
    NODE* M;
    if(master->value > child->value)
    {
        M=master;
        master=child;
        child=M;
    }
    else{
       child->next=master->child;
        master->child=child;
        master->degree++;
        
        int i=0;
        while((HEAP1->array[i])!=child && i < size_of_array)
        {
            i=i+1;
        }
        HEAP1->array[i]=NULL;
        OSMemPut((OS_MEM*)&MemoryCB_heap, (void*)HEAP1->array[i],(OS_ERR*)&err);
        int k=0;
        
        for(k=i;k<HEAP1->entries;k++)
        {
            HEAP1->array[k]=HEAP1->array[k+1];
        }
        HEAP1->array[k]=0;               //delete the pointer to the root of this tree and free the pointer memory allocated.
        HEAP1->entries--;
        return master;  
    }
        //return the pointer to the newly merged tree
}



/******** AFTER A TREE IN THE HEAP IS DELETED, REFLECT THE CHANGES IN THE NUMBER OF ROOT POINTERS IN THE ARRAY***************************/
void readjusting_array(NODE* node)
{
int i=0;
OS_ERR err;
while((HEAP1->array[i])!=node && i < size_of_array)
{
    i=i+1;
}
HEAP1->array[i]=NULL;
OSMemPut((OS_MEM*)&MemoryCB_heap, (void*)HEAP1->array[i],(OS_ERR*)&err);
int k=0;
for(k=i;k<HEAP1->entries;k++)
{
    HEAP1->array[k]=HEAP1->array[k+1];
}
HEAP1->array[k]=0;
HEAP1->entries--;
}
/*
*********************************************************************************************************
*       EXTRACTING THE MINIMUM NODE - CUTS THE NODE FROM THE TREE AND PROVIDES POINTER TO THAT NODE
*
* Description : Splay top-down approach
*
* Arguments   : none
*
* Returns     : none
*
* Notes       : Application should not use this function , update the total number of tasks used in os.h
*               
*********************************************************************************************************
*/


NODE* extract_min()
{
    NODE* temp;
    int i=0;
    if(HEAP1==NULL)                      //Heap is empty. Nothing to extract
    return NULL;
    else
    {
       temp=HEAP1->min;
        
       if(temp->child==NULL)
        {
            while((HEAP1->array[i])!=temp && i < size_of_array)
            {
              i=i+1;
             }
        readjusting_array(HEAP1->array[i]);
        find_min();
        return temp;
        }
      else
      {
        NODE* temp1=HEAP1->min->child;
        NODE* temp1_return=HEAP1->min;

        readjusting_array(HEAP1->min);
        NODE* temp_next;
        
        while(temp1->next!=NULL)
        {
        create_tree_roots(temp1);
        temp_next=temp1->next;
        temp1->next=NULL;
        heap_insert(temp1);
        temp1=temp_next;
        }
        create_tree_roots(temp1);
        temp1->next=NULL;
        heap_insert(temp1);
        temp1=NULL;
        find_min();
        return (temp1_return);
      }
  }
   
}
/*
*********************************************************************************************************
*                                       FREE THE EXTRACTED MINIMUM NODE'S MEMORY
*
* Description : Splay top-down approach
*
* Arguments   : none
*
* Returns     : none
*
* Notes       : Application should not use this function , update the total number of tasks used in os.h
*               
*********************************************************************************************************
*/
void free_node(NODE* temp)
{
  OS_ERR err1;
  OSMemPut((OS_MEM*)&MemoryCB_heap_node, (void*)temp,(OS_ERR*)&err1);;
}


