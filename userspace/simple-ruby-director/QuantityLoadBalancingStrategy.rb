# Helper class, counting tasks assigned per node
class PerNodeTaskCounter
    def initialize
        @counters = {}
        @lock = Monitor.new
	@pidToNode = {}
    end
    
    def addPid(node, pid)
        @lock.synchronize { 
	    if ( @pidToNode.include?(pid) )
	      # Nothing to do if node remained same
	      return if ( @pidToNode[pid] == node )
	      removePid(@pidToNode[pid], pid)
	    end
	  
            pids = @counters[node.id]
            if !pids
                pids = Set.new 
                @counters[node.id] = pids
            end            
            pids.add(pid)
	
	    @pidToNode[pid] = node
        }
    end
    
    def removePid(node, pid)
        @lock.synchronize { 
            pids = @counters[node.id]
            return if !pids
            pids.delete(pid)
	    @pidToNode.delete(pid)
        }        
    end
    
    def taskForked(node, childPid, parentPid)
        parentTask = nil
        @lock.synchronize { 
	    pids = @counters[node.id]
	                  
	    if ( pids && pids.include?(parentPid) )
		addPid(node, childPid)
	    end	                  
	}
	    	
	return parentTask
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
        # Minimum tasks running locally we want .. currently equals to core counts, could consider core count + 1?
        @minimumTasksLocal = @nodeRepository.selfNode.staticNodeInfo.coresCount
#        @minimumTasksLocal = 0 # Comment this out, testing only.. prefered way for testing is to use EMIG=1 env prop
        # Minimum tasks runnign on a remote node we want
        @minimumTasksRemote = 5
        # Fallback load balancing strategy, in case minimum task guarantees are satisfied
        @nestedLoadBalancer = RoundRobinBalancingStrategy.new(nodeRepository, membershipManager)
        
        @counter = PerNodeTaskCounter.new
	
	@log = nil
    end
    
    # Return ID of the node where the process shall be migrated
    # Returns nil, if no migration should be performed        
    def findMigrationTarget(pid, uid, name, args, envp, emigPreferred)        
        return nil if !@membershipManager.coreManager # Not a core node?
        detachedNodes = @membershipManager.coreManager.detachedNodes
        
        bestTarget = findBestTarget(pid, uid, name, args, envp, emigPreferred, detachedNodes)
	
        #puts "Best target #{bestTarget} for name #{name}."
	# Temporary hack.. we count only tasks that are migrateable somewhere... TODO: Instead introduce either some filter for counting tasks, or count based on their CPU usage (unlikely.. to difficult for short term)
        updateCounter(bestTarget, name, pid) if UserConfiguration.getConfig(uid).canMigrateSomewhere(name)
	flushDebugLog()
        bestTarget
    end    

    # Callback from task repository
    def newTask(task)
        # Nothing to be done
    end

    def taskFork(task, parentTask)
       @counter.taskForked(parentTask.executionNode, task.pid, parentTask.pid); 
       $log.debug("Forked task #{parentTask.pid} to #{task.pid}. Post-fork count: #{@counter.getCount(parentTask.executionNode)}. Node: #{parentTask.executionNode}")
    end

    # Callback from task repository
    def taskExit(task, exitCode)            
        @counter.removePid(task.executionNode, task.pid)
	$log.debug("Removing pid task #{task.pid}. Post-rem count: #{@counter.getCount(task.executionNode)}. Node: #{task.executionNode}")
    end
    
    def startDebuggingToFile(logname)
      @log = File.new("#{Director::LOG_DIR}/#{logname}", "w")      
    end
    
    def stopDebugging()
      @log = nil
    end
    
private
    def flushDebugLog()
      return if !@log
      
      @log.flush
    end

    def debugDecision(pid, targetNode)
      return if !@log
      
      debugDumpState()
      if ( targetNode.id != @nodeRepository.selfNode.id )
	@log.write("Migrating #{pid} to #{targetNode.ipAddress}\n");
      else
	@log.write("#{pid} kept locally\n");
      end
    end

    def debugDumpState()
      return if !@log
      
      @log.write("Local Node: #{@counter.getCount(@nodeRepository.selfNode)}\n|");
      @nodeRepository.eachNode { |node|
	  @log.write(" Node #{node.ipAddress}: #{@counter.getCount(node)} |");
      }
      @log.write("\n");
    end      

    def updateCounter(slotIndex, name, pid)            
        if ( !slotIndex )
	   $log.debug("Adding pid task #{pid} (#{name}) to self. Pre-add count: #{@counter.getCount(@nodeRepository.selfNode)}")
	   debugDecision(pid, @nodeRepository.selfNode)
           @counter.addPid(@nodeRepository.selfNode, pid) 	   
        else
	   $log.debug("Adding pid task #{pid} (#{name}) to slot #{slotIndex}. Pre-add count: #{@counter.getCount(@membershipManager.coreManager.detachedNodes[slotIndex])}" )
	   debugDecision(pid, @membershipManager.coreManager.detachedNodes[slotIndex])
           @counter.addPid(@membershipManager.coreManager.detachedNodes[slotIndex], pid)	   
        end
    end

    def keepLocal()
        @counter.getCount(@nodeRepository.selfNode) < @minimumTasksLocal
    end
    
    def findBestTarget(pid, uid, name, args, envp, emigPreferred, detachedNodes)
        #puts "Local task count #{@counter.getCount(@nodeRepository.selfNode)}"
        return nil if !emigPreferred && keepLocal()
        best = TargetMatcher.performMatch(pid, uid, name, detachedNodes) { |node|
	    taskCount = @counter.getCount(node)
            # Note that taskCount has to be returned negative, so that less tasks is better candidate!
	    # TODO: This is STUPID! If there are too many tasks then we fallback-to next strategy, but that strategy does not behave well and we get a HUGE load imabalance!
            taskCount < @minimumTasksRemote ? -taskCount : nil
        }                
        
        # Not found best? Delegate further	
        return @nestedLoadBalancer.findMigrationTarget(pid, uid, name, args, envp, emigPreferred) if !best
        # Found best
        return best
    end    
    
end