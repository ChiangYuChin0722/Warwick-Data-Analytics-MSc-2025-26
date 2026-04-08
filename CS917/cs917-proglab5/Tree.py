class BinaryTreeNode():

    def __init__(self,value):
        self.value = value
        self.leftChild = None
        self.rightChild = None

    def getLeftChild(self):
        return self.leftChild

    def getRightChild(self):
        return self.rightChild

    def getValue(self):
        return self.value

    def setLeftChild(self,node):
        self.leftChild = node

    def setRightChild(self,node):
        self.rightChild = node

    def setValue(self,value):
        self.value = value        


class BinaryTree():
    def __init__(self,rootValue):
        self.root = BinaryTreeNode(rootValue)

    def getRootNode(self):
        return self.root

    def insertAtRoot(self,value):
        self.insert(self.root,value)

    def insert(self,parent,value):
        if parent is None:
            return
        if value < parent.getValue():
            if parent.getLeftChild() is None:
                parent.setLeftChild(BinaryTreeNode(value))
            else:
                self.insert(parent.getLeftChild(),value)
        else:
            if parent.getRightChild() is None:
                parent.setRightChild(BinaryTreeNode(value))
            else:
                self.insert(parent.getRightChild(),value)

    def BFS(self):
        output = []
        queue = [self.root]
        while queue:
            current = queue.pop(0)
            output.append(current.getValue())
            if current.getLeftChild() is not None:
                queue.append(current.getLeftChild())
            if current.getRightChild() is not None:
                queue.append(current.getRightChild())
        return output
    
    # ----------------- DFS Inorder -----------------
    def DFSInOrder(self):
        output = []
        def traverse(node):
            if node is None:
                return
            traverse(node.getLeftChild())
            output.append(node.getValue())
            traverse(node.getRightChild())
        traverse(self.root)
        return output
    
    # ----------------- DFS Preorder -----------------
    def DFSPreOrder(self):
        output = []
        def traverse(node):
            if node is None:
                return
            output.append(node.getValue())
            traverse(node.getLeftChild())
            traverse(node.getRightChild())
        traverse(self.root)
        return output
    
    # ----------------- DFS Postorder -----------------
    def DFSPostOrder(self):
        output = []
        def traverse(node):
            if node is None:
                return
            traverse(node.getLeftChild())
            traverse(node.getRightChild())
            output.append(node.getValue())
        traverse(self.root)
        return output


def main():
    tree = BinaryTree(10)
    tree.insertAtRoot(5)
    tree.insertAtRoot(15)
    tree.insertAtRoot(12)
    tree.insertAtRoot(18)
    tree.insertAtRoot(3)

    print("BFS:", tree.BFS())

    print("Inorder:", tree.DFSInOrder())
    print("Preorder:", tree.DFSPreOrder())
    print("Postorder:", tree.DFSPostOrder())


if __name__ == "__main__":
    main()
