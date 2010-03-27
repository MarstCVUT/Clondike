# Helper class, counting tasks assigned per node
class PerNodeTaskCounter
    def initialize
        @counters = {}
        @lock = Mutex.new
    end
    
    def addPid(node, pid)
        @lock.synchronize { 
            pids = @counters[node.id]
            if !pids
                pids = Set.new 
                @counters[node.id] = pids
            end            
            pids.add(pid)
        }
    end
    
    def removePid(node, pid)
        @lock.synchronize { 
            pids = @counters[node.id]
            return if !pids
            pids.delete(pid)
        }        
    end
    
    def getCount(node)
        res = nil
        @lock.synchronize { 
            pids = @counters[node.id]
            if ( !pids )
                res = 0
            else 
                res = pids.size()
            end    
        }
        res
    end
end

# This load balancing strategy tries to balance load based on a quantity
# of tasks send to individual nodes
class QuantityLoadBalancingStrategy

    def initialize(nodeRepository, membershipManager)
        @nodeRepository = nodeRepository
        @membershipManager = membershipManager
        # Minimum tasks running locally we want
        @minimumTasksLocal = @nodeRepository.selfNode.staticNodeInfo.coresCount
        @minimumTasksLocal = 0 # TODO: Uncomment this, testing only
        # Minimum tasks runnign on a remote node we want
        @minimumTasksRemote = 5
        # Fallback load balancing strategy, in case minimum task guarantees are satisfied
        @nestedLoadBalancer = CpuLoadBalancingStrategy.new(nodeRepository, membershipManager)
        
        @counter = PerNodeTaskCounter.new
    end
    
    # Return ID of the node where the process shall be migrated
    # Returns nil, if no migration should be performed        
    def findMigrationTarget(pid, uid, name, args, envp)        
        return nil if !@membershipManager.coreManager # Not a core node?
        detachedNodes = @membershipManager.coreManager.detachedNodes
        
        bestTarget = findBestTarget(pid, uid, name, args, envp, detachedNodes)
        #puts "Best target #{bestTarget} for name #{name}."
        updateCounter(bestTarget, pid)
        bestTarget
    end    

    # Callback from task repository
    def newTask(task)
        # Nothing to be done
    end
    
    # Callback from task repository
    def taskExit(task, exitCode)    
        @counter.removePid(task.executionNode, task.pid)
    end
    
private
    def updateCounter(slotIndex, pid)
        if ( !slotIndex )
           @counter.addPid(@nodeRepository.selfNode, pid) 
        else
           @counter.addPid(@membershipManager.coreManager.detachedNodes[slotIndex], pid)
        end
    end

    def keepLocal()
        @counter.getCount(@nodeRepository.selfNode) < @minimumTasksLocal
    end
    
    def findBestTarget(pid, uid, name, args, envp, detachedNodes)
        #puts "Local task count #{@counter.getCount(@nodeRepository.selfNode)}"
        return nil if keepLocal
        best = TargetMatcher.performMatch(pid, uid, name, detachedNodes) { |node|
	    taskCount = @counter.getCount(node)
            # Note that taskCount has to be returned negative, so that less tasks is better candidate!
            taskCount < @minimumTasksRemote ? -taskCount : nil
        }                
        
        # Not found best? Delegate further
        return @nestedLoadBalancer.findMigrationTarget(pid, uid, name, args, envp) if !best
        # Found best
        return best
    end    
    
end