require 'UserConfiguration'

# Helper class used for target matching in migration strategies
class TargetMatcher
    # Performs the matching
    # Accepts block param.. that calculates quality of the node candidate
    #                     - returns nil if the node cannot be used
    #                     - The higher number the better
    def TargetMatcher.performMatch(pid, uid, name, candidateNodes)
        target = nil
        targetQuality = nil
        userConfig = UserConfiguration.getConfig(uid)
        candidateNodes.each_index do |index|
            node = candidateNodes[index]            
            #puts "Index: #{index} node #{node} #{node ? node.nodeInfo : "No Node"} #{node && node.nodeInfo ? node.nodeInfo.maximumAccept : "No Node-Info"}... #{node.id}"
            next if !canMigrateTo(pid, uid, name, node)
            nodeQuality = yield node                         
            if ( nodeQuality != nil && (targetQuality == nil || nodeQuality > targetQuality) ) then                
                targetQuality = nodeQuality
                target = index
            end
        end
                
        target
    end
    
    def TargetMatcher.canMigrateTo(pid, uid, name, node)
      userConfig = UserConfiguration.getConfig(uid)
      return node && node.nodeInfo && node.nodeInfo.maximumAccept >= 1 && userConfig.canMigrateTo(name, node.id)
    end
end