/*
*********************************************************************************************************
*                                              MUTEX DATA STRUCTURE
*********************************************************************************************************
*/
/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/
#include <os.h>
#include <stdlib.h>

AVL_NODE2* avl_root2;
AVL_NODE2* mintasklevel;
CPU_INT32U *MemoryPartition_avl2 [5][10];
OS_MEM MemoryCB_avl2;

void Tree234Init(void)
{
  OS_ERR      err;
  OSMemCreate((OS_MEM*)&MemoryCB_avl2, (CPU_CHAR*)"234_tree", &MemoryPartition_avl2[0][0], (OS_MEM_QTY)(5), (OS_MEM_SIZE)(10*sizeof(CPU_INT32U)), &err);
  avl_root2 = NULL;
  mintasklevel = NULL;
}

OS_TASK_DEADLINE height2(AVL_NODE2 *N)
{
    if (N == NULL)
        return 0;
    return N->height2;
}

OS_TASK_DEADLINE max2(OS_TASK_DEADLINE a, OS_TASK_DEADLINE b)
{
    return (a > b)? a : b;
}


AVL_NODE2* AVL_newNode2( OS_TCB* tcb_pointer, OS_TASK_DEADLINE preemption_threshold )
{
  OS_ERR err;
    AVL_NODE2* node = (AVL_NODE2*) OSMemGet((OS_MEM*)&MemoryCB_avl2, (OS_ERR*)&err);;
    node->preemption_threshold   = preemption_threshold;
	node->tcb_pointer = tcb_pointer;
    node->left   = NULL;
    node->right  = NULL;
    node->height2 = 1;  // new node is initially added at leaf
    return(node);
}


AVL_NODE2 *rightRotate2(AVL_NODE2 *y)
{
    AVL_NODE2 *x = y->left;
    AVL_NODE2 *T2 = x->right;
    
    // Perform rotation
    x->right = y;
    y->left = T2;
    
    // Update height2s
    y->height2 = max2(height2(y->left), height2(y->right))+1;
    x->height2 = max2(height2(x->left), height2(x->right))+1;
    
    // Return new root
    return x;
}


AVL_NODE2 *leftRotate2(AVL_NODE2 *x)
{
    AVL_NODE2 *y = x->right;
    AVL_NODE2 *T2 = y->left;
    
    // Perform rotation
    y->left = x;
    x->right = T2;
    
    //  Update height2s
    x->height2 = max2(height2(x->left), height2(x->right))+1;
    y->height2 = max2(height2(y->left), height2(y->right))+1;
    
    // Return new root
    return y;
}


CPU_INT08S getBalance2(AVL_NODE2 *N)
{
    if (N == NULL)
        return 0;
    return height2(N->left) - height2(N->right);
}

AVL_NODE2* InsertBlkTask(AVL_NODE2* node,  OS_TCB* tcb_pointer, OS_TASK_DEADLINE preemption_threshold)
{
    
    if (node == NULL)
        return(AVL_newNode2(tcb_pointer, preemption_threshold));
    
    if (preemption_threshold < node->preemption_threshold)
        node->left  = InsertBlkTask(node->left, tcb_pointer, preemption_threshold);
    else
        node->right = InsertBlkTask(node->right, tcb_pointer, preemption_threshold);
    
    
    /* 2. Update height2 of this ancestor node */
    node->height2 = 1 + max2(height2(node->left),
                           height2(node->right));
    
    /* 3. Get the balance factor of this ancestor
     node to check whether this node became
     unbalanced */
    CPU_INT08S balance = getBalance2(node);
    
    // If this node becomes unbalanced, then there are 4 cases
    
    // Left Left Case
    if (balance > 1 && preemption_threshold < node->left->preemption_threshold)
        return rightRotate2(node);
    
    // Right Right Case
    if (balance < -1 && preemption_threshold > node->right->preemption_threshold)
        return leftRotate2(node);
    
    // Left Right Case
    if (balance > 1 && preemption_threshold > node->left->preemption_threshold)
    {
        node->left =  leftRotate2(node->left);
        return rightRotate2(node);
    }
    
    // Right Left Case
    if (balance < -1 && preemption_threshold < node->right->preemption_threshold)
    {
        node->right = rightRotate2(node->right);
        return leftRotate2(node);
    }
    //mintasklevel = MinTaskLevel(avl_root2);
    /* return the (unchanged) node pointer */
    return node;
}


AVL_NODE2 * MinTaskLevel(AVL_NODE2* node)
{
    AVL_NODE2* current = node;
    while (current->left != NULL)
        current = current->left;
    
    return current;
}


AVL_NODE2* Delblocktask(AVL_NODE2* root,  OS_TASK_DEADLINE preemption_threshold)
{
    // STEP 1: PERFORM STANDARD BST DELETE
    OS_ERR err;
    if (root == NULL)
        return root;
    
    if ( preemption_threshold < root->preemption_threshold )
        root->left = Delblocktask(root->left, preemption_threshold);
    
    else if( preemption_threshold > root->preemption_threshold )
        root->right = Delblocktask(root->right, preemption_threshold);
    
    else
    {
        // node with only one child or no child
        if( (root->left == NULL) || (root->right == NULL) )
        {
            AVL_NODE2 *temp = root->left ? root->left : root->right;
            
            // No child case
            if (temp == NULL)
            {
                temp = root;
                root = NULL;
            }
            else // One child case
                *root = *temp; // Copy the contents of// the non-empty child
            OSMemPut((OS_MEM*)&MemoryCB_avl2, (void*)temp,(OS_ERR*)&err);
        }
        else
        {
            // node with two children: Get the inorder
            // successor (smallest in the right subtree)
            AVL_NODE2* temp = MinTaskLevel(root->right);
            
            // Copy the inorder successor's data to this node
            root->preemption_threshold = temp->preemption_threshold;
            
            // Delete the inorder successor
            root->right = Delblocktask(root->right, temp->preemption_threshold);
        }
    }
    
    // If the tree had only one node then return
    if (root == NULL)
        return root;
    
    // STEP 2: UPDATE height2 OF THE CURRENT NODE
    root->height2 = 1 + max2(height2(root->left),
                           height2(root->right));
    
    // STEP 3: GET THE BALANCE FACTOR OF THIS NODE (to
    // check whether this node became unbalanced)
    CPU_INT08S balance = getBalance2(root);
    
    // If this node becomes unbalanced, then there are 4 cases
    
    // Left Left Case
    if (balance > 1 && getBalance2(root->left) >= 0)
        return rightRotate2(root);
    
    // Left Right Case
    if (balance > 1 && getBalance2(root->left) < 0)
    {
        root->left =  leftRotate2(root->left);
        return rightRotate2(root);
    }
    
    // Right Right Case
    if (balance < -1 && getBalance2(root->right) <= 0)
        return leftRotate2(root);
    
    // Right Left Case
    if (balance < -1 && getBalance2(root->right) > 0)
    {
        root->right = rightRotate2(root->right);
        return leftRotate2(root);
    }
    //mintasklevel = MinTaskLevel(avl_root2);
    return root;
}