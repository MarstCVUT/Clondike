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
    
    def taskNodeChange(oldNode, newNode, pid)
	addPid(newNode, pid)
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
    
    def getRemoteCount(selfNode)
        res = 0
        @lock.synchronize { 
	    @counters.each { |nodeId, pids|
	      next if nodeId == selfNode.id
			    
	      if ( pids )
		  res = res + pids.size()
	      end
	    }
        }
        return res      
    end
    
    # Gets a (copy) snapshot of pids associated with node at current time
    def getPidsSnapshot(node)
        res = nil
        @lock.synchronize { 
            pids = @counters[node.id]
            if ( !pids )
                res = nil
            else 
                res = pids.clone()
            end    
        }
        res      
    end
end

# This load balancing strategy tries to balance load based on a quantity
# of tasks send to individual nodes
class QuantityLoadBalancingStrategy

    def initialize(nodeRepository, membershipManager, taskRepository, predictor)
        @nodeRepository = nodeRepository
        @membershipManager = membershipManager
	@taskRepository = taskRepository
	@predictor = predictor
        # Minimum tasks running locally we want .. currently equals to core counts, could consider core count + 1?
        @defaultMinimumTasksLocal = @nodeRepository.selfNode.staticNodeInfo.coresCount
#        @minimumTasksLocal = 0 # Comment this out, testing only.. prefered way for testing is to use EMIG=1 env prop
        # Minimum tasks runnign on a remote node we want
        @minimumTasksRemote = 200
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
    
    # Try to rebalance only in case there are more local tasks than 
    def findRebalancing()	  
	  detachedNodes = @membershipManager.coreManager.detachedNodes
	  
	  rebalancePlan = nil
	  pids = @counter.getPidsSnapshot(@nodeRepository.selfNode)
	  pidsCount = 0
	  pidsCount = pids.size if pids
#	  $log.debug("Find rebalance with local load pids count #{pidsCount}")
	  if ( pidsCount > getCurrentLocalMinimum() )
	    rebalancePlan = generateRebalancePlan(pids, detachedNodes)
	  end      
	  return rebalancePlan	 
    end

    # Callback from task repository
    def newTask(task)
        # Nothing to be done
    end

    def taskFork(task, parentTask)
       @counter.taskForked(parentTask.executionNode, task.pid, parentTask.pid); 
#       $log.debug("Forked task #{parentTask.pid} to #{task.pid}. Post-fork count: #{@counter.getCount(parentTask.executionNode)}. Node: #{parentTask.executionNode}")
    end

    # Callback from task repository
    def taskExit(task, exitCode)            
        @counter.removePid(task.executionNode, task.pid)
#	$log.debug("Removing pid task #{task.pid}. Post-rem count: #{@counter.getCount(task.executionNode)}. Node: #{task.executionNode}")
    end
    
    # Callback from task repo
    def taskNodeChanged(task, oldNode)
      @counter.taskNodeChange(oldNode, task.executionNode, task.pid)
    end
    
    def startDebuggingToFile(logname)
      @log = File.new("#{Director::LOG_DIR}/#{logname}", "w")      
    end
    
    def stopDebugging()
      @log = nil
    end
    
    # Callback method as a local task count provider (used in NodeInfoProvider)
    def localTaskCount()
      @counter.getCount(@nodeRepository.selfNode)
    end
private
    def getCurrentLocalMinimum()      
      remoteCount = @counter.getRemoteCount(@nodeRepository.selfNode)
      return 0 if ( remoteCount > 3 && @membershipManager.coreManager.detachedNodes.size > 2 )
      return @defaultMinimumTasksLocal
    end

    def flushDebugLog()
      return if !@log
      
      @log.flush
    end

    def debugDecision(pid, targetNode)
      return if !@log
      
      debugDumpState()
      if ( targetNode && targetNode.id != @nodeRepository.selfNode.id )
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
#	   $log.debug("Adding pid task #{pid} (#{name}) to self. Pre-add count: #{@counter.getCount(@nodeRepository.selfNode)}")
	   debugDecision(pid, @nodeRepository.selfNode)
           @counter.addPid(@nodeRepository.selfNode, pid) 	   
        else
#	   $log.debug("Adding pid task #{pid} (#{name}) to slot #{slotIndex}. Pre-add count: #{@counter.getCount(@membershipManager.coreManager.detachedNodes[slotIndex])}" )
	   debugDecision(pid, @membershipManager.coreManager.detachedNodes[slotIndex])
           @counter.addPid(@membershipManager.coreManager.detachedNodes[slotIndex], pid)	   
        end
    end

    def keepLocal()
        @counter.getCount(@nodeRepository.selfNode) < getCurrentLocalMinimum()
    end
    
    def findBestTarget(pid, uid, name, args, envp, emigPreferred, detachedNodes)
        #puts "Local task count #{@counter.getCount(@nodeRepository.selfNode)}"
        return nil if !emigPreferred && keepLocal()
	task = @taskRepository.getTask(pid)
	# Do not emigrate non-preemptively master tasks (it may be worth considering, but now we have a problems with it)
	return nil if task and task.hasClassification(MasterTaskClassification.new()) 
	
        best = findBestTargetRemoteOnly(pid, uid, name, detachedNodes)
        # Not found best? Delegate further	
        return @nestedLoadBalancer.findMigrationTarget(pid, uid, name, args, envp, emigPreferred) if !best
        # Found best
        return best
    end                
    
    def findBestTargetRemoteOnly(pid, uid, name, detachedNodes)
        best = TargetMatcher.performMatch(pid, uid, name, detachedNodes) { |node|
	    taskCount = @counter.getCount(node)
            # Note that taskCount has to be returned negative, so that less tasks is better candidate!
            taskCount < @minimumTasksRemote ? -taskCount : nil
        }                      
	return best
    end
    
    def generateRebalancePlan(pids, detachedNodes)
      # TODO: Consider how long has the task already been on local node.. prefer to emigrate shorter time tasks as they may have been migrated home and hence required less memory shift (not all memory is loaded immediately)
      #In addition we may simply consider moving tasks with less volume of absolute memory
      plan = {}   
      maxEmigrateCount = pids.size - getCurrentLocalMinimum()
      pids.each { |pid|
	  task = @taskRepository.getTask(pid)
	  if task && task.hasClassification(MigrateableLongTermTaskClassification.new()) && !task.hasClassification(MasterTaskClassification.new())
	      target = findBestTargetRemoteOnly(pid, task.uid, task.name, detachedNodes)	          
	      if ( target )
		pids.delete(pid)
		plan[pid] = target
                @counter.addPid(detachedNodes[target], pid) # Move task in tracking to a new node.. if emigration fails we'll get notified later
                maxEmigrateCount = maxEmigrateCount - 1
	      end
                
              break if maxEmigrateCount < 1
	  end	    	    
      }
	
      return plan      
    end    
end
