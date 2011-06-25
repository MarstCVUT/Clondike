#This class keeps track of all nodes we've ever heard about
class NodeRepository
    # Hash of all known nodes. Node id is the key in hash
    attr_reader :nodes
    # Information about "this" node    
    attr_reader :selfNode

    def initialize(currentNode)
        @nodes = {}
        @selfNode = currentNode
        @lock = Mutex.new
    end

    def addOrReplaceNode node        
        @lock.synchronize {
          @nodes[node.id] = node
        }
    end

    # Do we really need that? ;)
    def removeNode node
        @lock.synchronize {
          @nodes[node.id] = nil
        }
    end

    def getNode nodeId
        @lock.synchronize {
          @nodes[nodeId]
        }
    end
    
    def getNodeWithIp(nodeIp)
        @lock.synchronize {
	    @nodes.values.each { |node|
		return node if ( node.ipAddress == nodeIp )
	    }
        }		      
    end
    
    # Iterates all 'remote' nodes
    def eachNode
        nodesClone = nil
        @lock.synchronize {
          nodesClone = @nodes.dup
        }
        
        nodesClone.values.each() { |node|  
            yield(node)
        }        
    end
    
    def getNodesCopy()
        @lock.synchronize {
          nodesClone = @nodes.dup
        }      
    end

    # Count of known 'remote' nodes. Does not include self
    def knownNodesCount
	return nodes.size
    end
    
    #Gets node by id. If there is no such a node, creates it and registers it
    #Returns array containing node, and indicator, whether this is a newly created
    #node or not
    def getOrCreateNode(nodeId, ipAddress)
        node = nil
        newNode = false
        @lock.synchronize {
          node = @nodes[nodeId]
          if !node then
              newNode = true
              node = Node.new(nodeId, ipAddress)
              @nodes[nodeId] = node
          end
        }
        [node, newNode]
    end
end