# This filter can be plugged between the NodeInfoProvider and its listener
# It filters the notification of changes to propage further only "important" changes
class SignificanceTracingFilter
    def initialize (listener)
        @listener = listener
        @lastInfo = nil
    end
    
    def notifyChange(nodeInfoWithId)
        nodeInfo = nodeInfoWithId.nodeInfo
        if ( isInfoChangeSignificant(nodeInfo) ) then
            @listener.notifyChange(nodeInfoWithId)
            @lastInfo = nodeInfo
        end
    end
    
private
     #Is the change significant enough to propagate it immediately?     
     def isInfoChangeSignificant(newInfo)         
         # First info => Immediately
         return true if @lastInfo == nil
         # Load changed more than 0.2
         return true if ((@lastInfo.load - newInfo.load).abs > 0.2)
         # Cpu usage changed more than by 10%
         ((@lastInfo.cpuUsage - newInfo.cpuUsage).abs > 10)
     end

end