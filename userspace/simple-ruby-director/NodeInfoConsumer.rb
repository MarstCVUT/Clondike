require 'set'

#This class is consuming information received by the info distribution
#strategy and distributing them to interested resources
class NodeInfoConsumer
    def initialize(nodeRepository, currentNodeId)
        @nodeRepository = nodeRepository
        @newNodeListeners = Set.new
	@updateListeners = Set.new
        @currentNodeId = currentNodeId
    end
    
    def registerNewNodeListener(listener)        
        @newNodeListeners.add(listener)
    end

    def registerUpdateListener(listener)        
        @updateListeners.add(listener)
    end

    # Called, when a new information is received
    def infoReceived(fromIp, info)
        case info
          when NodeInfoWithId
            #We are not interested in info about us ;)
            return if info.nodeId == @currentNodeId

            dispatchNodeInfo(fromIp, info)
          else
            puts "Unknown type of info arrived #{info.class}"
        end
    end
    
private
    def dispatchNodeInfo(fromIp, nodeInfoWithId)     
        node, isNew = @nodeRepository.getOrCreateNode(nodeInfoWithId.nodeId, fromIp)      
        node.updateInfo(nodeInfoWithId.nodeInfo)      
#	puts "UPDATED #{node} was new #{isNew} -> Node info #{nodeInfoWithId.nodeInfo}"
        notifyNewNode(node) if isNew 
        notifyUpdate(node, nodeInfoWithId.nodeInfo)
    end
    
    def notifyUpdate(node, nodeInfo)
        @updateListeners.each { |listener| listener.notifyUpdate(node, nodeInfo) }
    end
    
    def notifyNewNode(node)
        @newNodeListeners.each { |listener| listener.notifyNewNode(node) }
    end
end