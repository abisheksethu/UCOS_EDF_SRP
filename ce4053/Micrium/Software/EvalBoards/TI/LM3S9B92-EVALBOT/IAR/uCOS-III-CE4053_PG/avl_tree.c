
#include<os.h>
#include<stdlib.h>

AVL_NODE* avl_root;
AVL_NODE* avl_min;

AVL_NODE* avl_root;
AVL_NODE* avl_min;
CPU_INT32U *MemoryPartition_avl [3][10];
OS_MEM MemoryCB_avl;

void avlTreeInit(void)
{
  OS_ERR      err;
  OS_MEM_QTY node_size = sizeof(CPU_INT32U);
  OSMemCreate((OS_MEM*)&MemoryCB_avl, (CPU_CHAR*)"avl_tree_node", &MemoryPartition_avl[0][0], (OS_MEM_QTY)(10), (OS_MEM_SIZE)(3*node_size), &err);
  avl_root = NULL;
  avl_min = NULL;
  
}

CPU_INT08U height(AVL_NODE *N)
{
    if (N == NULL)
        return 0;
    return N->height;
}

CPU_INT08U max(CPU_INT08U a, CPU_INT08U b)
{
    return (a > b)? a : b;
}


AVL_NODE* AVL_newNode( OS_MUTEX* mutex_pointer, CPU_INT08U resource_ceiling)
{
  OS_ERR err;
    AVL_NODE* node = (AVL_NODE*) OSMemGet((OS_MEM*)&MemoryCB_avl, (OS_ERR*)&err);;
    node->resource_ceiling   = resource_ceiling;
	node->mutex_pointer = mutex_pointer;
    node->left   = NULL;
    node->right  = NULL;
    node->height = 1;  // new node is initially added at leaf
    return(node);
}


AVL_NODE *rightRotate(AVL_NODE *y)
{
    AVL_NODE *x = y->left;
    AVL_NODE *T2 = x->right;
    
    // Perform rotation
    x->right = y;
    y->left = T2;
    
    // Update heights
    y->height = max(height(y->left), height(y->right))+1;
    x->height = max(height(x->left), height(x->right))+1;
    
    // Return new root
    return x;
}


AVL_NODE *leftRotate(AVL_NODE *x)
{
    AVL_NODE *y = x->right;
    AVL_NODE *T2 = y->left;
    
    // Perform rotation
    y->left = x;
    x->right = T2;
    
    //  Update heights
    x->height = max(height(x->left), height(x->right))+1;
    y->height = max(height(y->left), height(y->right))+1;
    
    // Return new root
    return y;
}


CPU_INT08S getBalance(AVL_NODE *N)
{
    if (N == NULL)
        return 0;
    return height(N->left) - height(N->right);
}

AVL_NODE* insert(AVL_NODE* node,  OS_MUTEX* mutex_pointer, CPU_INT08U resource_ceiling)
{
    
    if (node == NULL)
        return(AVL_newNode(mutex_pointer, resource_ceiling));
    
    if (resource_ceiling <= node->resource_ceiling)
        node->left  = insert(node->left, mutex_pointer, resource_ceiling);
    else
        node->right = insert(node->right, mutex_pointer, resource_ceiling);
    
    
    /* 2. Update height of this ancestor node */
    node->height = 1 + max(height(node->left),
                           height(node->right));
    
    /* 3. Get the balance factor of this ancestor
     node to check whether this node became
     unbalanced */
    CPU_INT08S balance = getBalance(node);
    
    // If this node becomes unbalanced, then there are 4 cases
    
    // Left Left Case
    if (balance > 1 && resource_ceiling < node->left->resource_ceiling)
        return rightRotate(node);
    
    // Right Right Case
    if (balance < -1 && resource_ceiling > node->right->resource_ceiling)
        return leftRotate(node);
    
    // Left Right Case
    if (balance > 1 && resource_ceiling > node->left->resource_ceiling)
    {
        node->left =  leftRotate(node->left);
        return rightRotate(node);
    }
    
    // Right Left Case
    if (balance < -1 && resource_ceiling < node->right->resource_ceiling)
    {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }
    avl_min = minValueNode(avl_root);
    /* return the (unchanged) node pointer */
    return node;
}


AVL_NODE * minValueNode(AVL_NODE* node)
{
    AVL_NODE* current = node;
    while (current->left != NULL)
        current = current->left;
    
    return current;
}


AVL_NODE* deleteNode(AVL_NODE* root,  CPU_INT08U resource_ceiling)
{
    // STEP 1: PERFORM STANDARD BST DELETE
    OS_ERR err;
    if (root == NULL)
        return root;
    
    if ( resource_ceiling <= root->resource_ceiling )
        root->left = deleteNode(root->left, resource_ceiling);
    
    else if( resource_ceiling > root->resource_ceiling )
        root->right = deleteNode(root->right, resource_ceiling);
    
    else
    {
        // node with only one child or no child
        if( (root->left == NULL) || (root->right == NULL) )
        {
            AVL_NODE *temp = root->left ? root->left : root->right;
            
            // No child case
            if (temp == NULL)
            {
                temp = root;
                root = NULL;
            }
            else 
            {// One child case
                *root = *temp; // Copy the contents of// the non-empty child
                 OSMemPut((OS_MEM*)&MemoryCB_avl, (void*)temp,(OS_ERR*)&err);
            }
        }
        else
        {
            // node with two children: Get the inorder
            // successor (smallest in the right subtree)
            AVL_NODE* temp = minValueNode(root->right);
            
            // Copy the inorder successor's data to this node
            root->resource_ceiling = temp->resource_ceiling;
            
            // Delete the inorder successor
            root->right = deleteNode(root->right, temp->resource_ceiling);
        }
    }
    
    // If the tree had only one node then return
    if (root == NULL)
        return root;
    
    // STEP 2: UPDATE HEIGHT OF THE CURRENT NODE
    root->height = 1 + max(height(root->left),
                           height(root->right));
    
    // STEP 3: GET THE BALANCE FACTOR OF THIS NODE (to
    // check whether this node became unbalanced)
    CPU_INT08S balance = getBalance(root);
    
    // If this node becomes unbalanced, then there are 4 cases
    
    // Left Left Case
    if (balance > 1 && getBalance(root->left) >= 0)
        return rightRotate(root);
    
    // Left Right Case
    if (balance > 1 && getBalance(root->left) < 0)
    {
        root->left =  leftRotate(root->left);
        return rightRotate(root);
    }
    
    // Right Right Case
    if (balance < -1 && getBalance(root->right) <= 0)
        return leftRotate(root);
    
    // Right Left Case
    if (balance < -1 && getBalance(root->right) > 0)
    {
        root->right = rightRotate(root->right);
        return leftRotate(root);
    }
    avl_min = minValueNode(avl_root);
    return root;
}


