/*
*********************************************************************************************************
*                                              TASK RECURSION LIST
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
#include <stdio.h>
#include <os.h>
/*
*********************************************************************************************************
*                                            GLOBAL VARIABLES
*********************************************************************************************************
*/
Tree * RecursionTree;
CPU_INT32U *MemoryPartition [10][10];
OS_MEM MemoryCB;
/*
*********************************************************************************************************
*                                          SPLAY TREE INITILIZATION
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

void SplayTreeInit(void)
{
  OS_ERR      err;
  OS_MEM_QTY node_size = sizeof(CPU_INT32U);
  OSMemCreate((OS_MEM*)&MemoryCB, (CPU_CHAR*)"Splay_tree_node", &MemoryPartition[0][0], (OS_MEM_QTY)(10), (OS_MEM_SIZE)(10*node_size), &err);
  RecursionTree = NULL;
}
/*
*********************************************************************************************************
*                                          SPLAY OPERATION
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
Tree * splay (OS_TASK_RELEASE_TIME i, Tree * t) {
/* Simple top down splay, not requiring i to be in the tree t.  */
/* What it does is described above.                             */
    Tree N, *l, *r, *y;
    if (t == NULL) return t;
    N.left = N.right = NULL;
    l = r = &N;

    for (;;) {
  if (i < t->release_time) {
      if (t->left == NULL) break;
      if (i < t->left->release_time) {
    y = t->left;                           /* rotate right */
    t->left = y->right;
    y->right = t;
    t = y;
    if (t->left == NULL) break;
      }
      r->left = t;                               /* link right */
      r = t;
      t = t->left;
  } else if (i > t->release_time) {
      if (t->right == NULL) break;
      if (i > t->right->release_time) {
    y = t->right;                          /* rotate left */
    t->right = y->left;
    y->left = t;
    t = y;
    if (t->right == NULL) break;
      }
      l->right = t;                              /* link left */
      l = t;
      t = t->right;
  } else {
      break;
  }
    }
    l->right = t->left;                                /* assemble */
    r->left = t->right;
    t->left = N.right;
    t->right = N.left;
    return t;
}
/*
*********************************************************************************************************
*                                          INSERT IN TASK RECURSION TREE
*
* Description : insert a new node to the tree, it supports insertion with same priority 
*
* Arguments   : release time, tree_pointer, tcb 
*
* Returns     : tree_pointer
*
* Notes       : 
*               
*********************************************************************************************************
*/
Tree * InsertRecTree(OS_TASK_RELEASE_TIME i, Tree * t, OS_TCB* block) {
/* Insert i into the tree t, unless it's already there.    */
/* Return a pointer to the resulting tree.                 */
    Tree * new1;
    OS_ERR  err;
    new1 = (Tree *) OSMemGet((OS_MEM*)&MemoryCB, (OS_ERR*)&err);
    if (new1 == NULL) {
      err = OS_ERR_Z;
    }
    
    new1->release_time = i;
    if (t == NULL) {
        new1->left = new1->right = NULL;
        for (int j = 0; j < NUM_OF_TASKS; j++){
          new1->p_tcb[j] = 0;
        }
        new1->entries = 0;
        new1->p_tcb[new1->entries] = block; 
        new1->entries++;
        return new1;
    }
    
    t = splay(i,t);
    
    if (i < t->release_time) {
      new1->left = t->left;
      new1->right = t;
      t->left = NULL;
      for (int j = 0; j < NUM_OF_TASKS; j++){
        new1->p_tcb[j] = 0;
      }
      new1->entries = 0;
      new1->p_tcb[new1->entries] = block; 
      new1->entries++;
      return new1;
    } 

    else if (i > t->release_time) {
      new1->right = t->right;
      new1->left = t;
      t->right = NULL;
      for (int j = 0; j < NUM_OF_TASKS; j++){
        new1->p_tcb[j] = 0;
      }
      new1->entries = 0;
      new1->p_tcb[new1->entries] = block; 
      new1->entries++;
      return new1;
    } 
    else {
      t->p_tcb[t->entries] = block; 
      t->entries++;
      /* We get here if it's already in the tree */
      /* Don't add it again                      */
      OSMemPut((OS_MEM*)&MemoryCB, (void*)new1,(OS_ERR*)&err);
      //free(new1);
      return t;
    }
}
/*
*********************************************************************************************************
*                                          DELETE NODE
*
* Description : delete the given node from the tree
*
* Arguments   : release time, tree_pointer 
*
* Returns     : tree_pointer
*
* Notes       : 
*               
*********************************************************************************************************
*/
Tree * DelRecTree(OS_TASK_RELEASE_TIME i, Tree * t) {
/* delete_nodes i from the tree if it's there.               */
/* Return a pointer to the resulting tree.              */
  Tree * x;
  OS_ERR  err;
  if (t==NULL) return NULL;
  t = splay(i,t);
  if (i == t->release_time) {               /* found it */
    if (t->left == NULL) {
        x = t->right;
    } else {
        x = splay(i, t->left);
        x->right = t->right;
    }
    OSMemPut((OS_MEM*)&MemoryCB, (void*)t,(OS_ERR*)&err);
    //free(t);
    return x;
    }
  else{

  }
  return t;                         /* It wasn't there */
  }
/*
*********************************************************************************************************
*                                          FIND MINIMUM FROM THE LIST
*
* Description : insert a new node to the tree
*
* Arguments   : tree_pointer
*
* Returns     : tree_pointer for the minimum node or left most leaf
*
* Notes       : Donot assign to root of the tree in application
*               
*********************************************************************************************************
*/
Tree* GetMinRecTree(Tree *t)
{
  if(t==NULL)
  {
    return t;
  }
  while(t->left!=NULL)
  {
    t=t->left;
  }
  return t;
}