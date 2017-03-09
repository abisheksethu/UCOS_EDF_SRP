#include<stdio.h>
#include<os.h>

SPLAYTREE_NODE* root_tree;
SPLAYTREE_NODE* min;

CPU_INT32U *MemoryPartition [10][10];
OS_MEM MemoryCB;

void SplayTreeInit(void)
{
  OS_ERR      err;
  OS_MEM_QTY node_size = sizeof(CPU_INT32U);
  OSMemCreate((OS_MEM*)&MemoryCB, (CPU_CHAR*)"Splay_tree_node", &MemoryPartition[0][0], (OS_MEM_QTY)(10), (OS_MEM_SIZE)(10*node_size), &err);
  root_tree = NULL;
}


//forward function declarations
void splay(SPLAYTREE_NODE*);

SPLAYTREE_NODE* create_node(OS_TASK_RELEASE_TIME node_release_time)
{
  OS_ERR  err;
  SPLAYTREE_NODE* new_node = (SPLAYTREE_NODE *) OSMemGet((OS_MEM*)&MemoryCB, (OS_ERR*)&err);  
    new_node->release_time=node_release_time;
    new_node->left_child=new_node->right_child=new_node->parent=NULL;
    new_node->entries=0;
    for(int i=0;i<NUM_OF_TASKS;i++)
    new_node->tcb_pointer_array[i]=0;
    return new_node;
}


void insert_node(SPLAYTREE_NODE* root, OS_TASK_RELEASE_TIME release_time, OS_TCB* tcb_pointer)
{
    if(root==NULL)
    {
    root_tree=create_node(release_time);
    root_tree->tcb_pointer_array[0]=tcb_pointer;
    root_tree->entries++;
    min=root_tree;
    }
    
    else
    {
    if(release_time==root->release_time && root->entries<NUM_OF_TASKS) //add these changes here
        {
            root->tcb_pointer_array[root->entries]=tcb_pointer;
            root->entries++;
        }
     else if(release_time < root->release_time)
        {
            if(root->left_child==NULL)
            {
                SPLAYTREE_NODE* new=create_node(release_time);
                root->left_child=new;   //make this node the left child
                new->parent=root;       //make the root its parent
                root=new;               //make this node as the new root now
                root->tcb_pointer_array[root->entries]=tcb_pointer;
                root->entries++;
                min=new;
            }
            else
            {
                insert_node(root->left_child, release_time, tcb_pointer);
            }
        }
        else
        {
            if(root->right_child==NULL)
            {
                SPLAYTREE_NODE* new=create_node(release_time);
                root->right_child=new;
                new->parent=root;
                root=new;
                root->tcb_pointer_array[root->entries]=tcb_pointer;
                root->entries++;
            }
            else
            {
                insert_node(root->right_child, release_time, tcb_pointer);
            }
        }
    }
}



void rotate_right(SPLAYTREE_NODE* pointer)
{
    if(pointer->parent==NULL)
        return;
    else
    {
        
    if(pointer->parent->parent!=NULL)
    {
    SPLAYTREE_NODE* temp=pointer->right_child;            //store pointer's right child.
    pointer->right_child=pointer->parent;       //make pointer's parent its right child
    pointer->parent->parent=pointer;            //pointer's now right child's parent is pointer
    pointer->parent=pointer->parent->parent;
    pointer->parent->left_child=pointer;
    pointer->right_child->left_child=temp;
        if(temp!=NULL)
        {
        temp->parent=pointer->right_child;
        }
    }
    
    else
    {
        SPLAYTREE_NODE* temp=pointer->right_child;            //store pointer's right child.
        pointer->right_child=pointer->parent;       //make pointer's parent its right child
        pointer->parent->parent=pointer;
        pointer->parent=NULL;//pointer's now right child's parent is pointer
        pointer->right_child->left_child=temp;
         if(temp!=NULL)
         {
        temp->parent=pointer->right_child;
         }
    }
}

}
void rotate_left(SPLAYTREE_NODE* pointer)
{
    if(pointer->parent==NULL)
      return;
    else
    {
    if(pointer->parent->parent!=NULL)
    {
    SPLAYTREE_NODE* temp=pointer->left_child;            //store pointer's right child.
    pointer->left_child=pointer->parent;       //make pointer's parent its right child
    pointer->parent->parent=pointer;            //pointer's now right child's parent is pointer
    pointer->parent=pointer->parent->parent;
    pointer->parent->right_child=pointer;
    pointer->left_child->right_child=temp;
    if(temp!=NULL)
    {
    temp->parent=pointer->left_child;
    }
        pointer->parent->right_child=pointer->left_child;
        pointer->left_child=pointer->parent;
        pointer->parent=pointer->parent->parent;
        //root_tree=pointer;
        pointer->left_child->parent=pointer;}
    else
    {
        SPLAYTREE_NODE* temp=pointer->left_child;            //store pointer's right child.
        pointer->left_child=pointer->parent;       //make pointer's parent its right child
        pointer->parent->parent=pointer;
        pointer->parent=NULL;                       //pointer's now right child's parent is pointer
        pointer->left_child->right_child=temp;
        if(temp!=NULL)
        {
        temp->parent=pointer->left_child;
        }
    
    }
}
}

void right_child_of_right_child(SPLAYTREE_NODE* pointer)
{
    pointer=pointer->parent;
    rotate_left(pointer);
    pointer=pointer->right_child;
    rotate_left(pointer);
    //root_tree=pointer;
}

void left_child_of_left_child(SPLAYTREE_NODE* pointer)
{
    pointer=pointer->parent;
    rotate_right(pointer);
    pointer=pointer->left_child;
    rotate_right(pointer);
    //root_tree=pointer;
}

void right_child_of_left_child(SPLAYTREE_NODE* pointer)
{
    rotate_left(pointer);
    rotate_right(pointer);
    //root_tree=pointer;
}

void left_child_of_right_child(SPLAYTREE_NODE* pointer)
{
    rotate_right(pointer);
    rotate_left(pointer);
    //root_tree=pointer;
}

signed int decision(SPLAYTREE_NODE* pointer)
{
if(root_tree==pointer)//it is the first node and root of the tree
        return 6;
else if(pointer->parent->parent!=NULL && pointer->parent->parent->left_child==pointer->parent && pointer->parent->left_child==pointer)
    return 0;
else if(pointer->parent->parent!=NULL && pointer->parent->parent->right_child==pointer->parent && pointer->parent->left_child==pointer)
    return 1;
else if(pointer->parent->parent!=NULL && pointer->parent->parent->right_child==pointer->parent && pointer->parent->right_child==pointer)
    return 2;
else if(pointer->parent->parent!=NULL && pointer->parent->parent->left_child==pointer->parent && pointer->parent->right_child==pointer)
    return 3;
else if(pointer->parent->parent==NULL && pointer->parent->left_child==pointer)
    return 4;
else if(pointer->parent->parent==NULL && pointer->parent->right_child==pointer)
    return 5;
else return -1;
}

void splay(SPLAYTREE_NODE* pointer)
{
signed int decide;
decide=decision(pointer);
if(decide!=6)
 {
//find out the node category
switch(decide)
{
case -1: break;
case 0: left_child_of_left_child(pointer);
        break;
case 1: left_child_of_right_child(pointer);
        break;
case 2: right_child_of_right_child(pointer);
        break;
case 3: right_child_of_left_child(pointer);
        break;
case 4: rotate_right(pointer);
        break;
case 5: rotate_left(pointer);
        break;
default: break;
}//end of switch case
}//end of while loop

 root_tree=pointer;
}


void delete_minimum() //delete the argument here
{
  OS_ERR err;
   if(min!=NULL)
   {
    splay(min);
    
    if(min->right_child==NULL)
    {
        root_tree=NULL;
        OSMemPut((OS_MEM*)&MemoryCB, (void*)min,(OS_ERR*)&err);
    }
    else
    {
        root_tree=min->right_child;
        root_tree->parent=NULL;
        OSMemPut((OS_MEM*)&MemoryCB, (void*)min,(OS_ERR*)&err);
        if(root_tree->left_child==NULL)
        min=root_tree;
        else
        {
            SPLAYTREE_NODE* temp=root_tree;
            while(temp->left_child!=NULL)
            {
                temp=temp->left_child;
            }
            min=temp;
            OSMemPut((OS_MEM*)&MemoryCB, (void*)temp,(OS_ERR*)&err);
        }
   }
   }
}





