#Information about OS task
class TaskInfo
    # Pid of the task
    attr_reader :pid
    # Effective user id of the task
    attr_reader :uid
    # Pointer to parent task (if known)
    attr_reader :parent
    # Name of the executable
    attr_reader :name
    # Arguments used on startup
    attr_reader :args
    # Time in millis, when the task was started (actually the time, when it was
    # registered to this component). Float, in seconds with millis as floating point fration
    attr_reader :startTime
    # Set on exit, can be used by exit listeners
    attr_accessor :endTime
    # Node, where the task originated from   
    attr_reader :homeNode    
    # Node, where is the task being executed at the moment (as far as we can say)    
    attr_reader :executionNode
    # Child tasks of this task
    attr_reader :children
    # Total count of forks of this task during its lifetime
    attr_reader :forkCount
    # Set of classifications assigned to task
    attr_reader :classifications
    
    def initialize(pid, uid, parent, name, args, homeNode)
        @pid = pid
        @uid = uid
	@parent = parent
        @name = name
        @args = args
        @homeNode = homeNode
        @executionNode = homeNode
        @startTime = Time.now.to_f
	@children = Set.new
	@classifications = Set.new
	@forkCount = 0
    end
    
    def updateExecutionNode(node)
        @executionNode = node
    end
        
    def addForkedChild(childPid)
	@forkCount = @forkCount + 1
	childTask = TaskInfo.new(childPid, @uid, self, @name, @args, @homeNode)
	@children.add(childTask)
	return childTask
    end
    
    def removeChild(taskInfo)
        @children.remove(taskInfo)
    end
    
    def addClassification(classification)
	@classifications.add(classification)
    end
    
    def hasClassification(classification)
	@classifications.include?(classification)
    end
    
    def copySurviveForkClassifications(toTask)
	@classifications.each { |classification|
	    toTask.addClassification(classification) if classification.surviveFork
	}
    end

    def copySurviveExecClassifications(toTask)
	@classifications.each { |classification|
	    toTask.addClassification(classification) if classification.surviveExec
	}
    end
    
    def getClassificationOfType(classificationClass)
	result = nil;
	@classifications.each { |classification|
	    if classification.class == classificationClass then
		result = classification
	        break;
	    end	             
	}
	return result
    end
    
    # Returns string representation of associated classifications
    def classifications_to_s	
	taskClassificationsString = @classifications.to_a.join(", ")
	return taskClassificationsString
    end
    
    def duration
      return nil if !@endTime
      return @endTime.to_f - @startTime.to_f
    end

    def ==(other)
        @pid == other.pid && @startTime == other.startTime
    end
end

# This class holds records about currently running tasks
class TaskRepository
    def initialize(nodeRepository, membershipManager)
        @tasks = {}
        # Patterns of executable names, that require full args to be retrieved
        @fullArgRequired = []
        # TODO: Currently hardcoded, but this shall be configurable by listeners
        #@fullArgRequired << Regexp.new("ld$")
	@fullArgRequired << Regexp.new(".*") # Need full args in all cases now
        @lock = Mutex.new
        @nodeRepository = nodeRepository
        @membershipManager = membershipManager
        @listeners = []
	@forkClassificators = []
	@execClassificators = []
    end

    def registerListener(listener)
        @listeners << listener
    end
    
    def addForkClassificator(classificator)
        @forkClassificators << classificator
    end
    
    def addExecClassificator(classificator)
        @execClassificators << classificator
    end
    
    def onFork(pid, parentPid)
        task = nil
        @lock.synchronize {
            task = @tasks[parentPid]
        }

	if ( task ) 
	  child = task.addForkedChild(pid)
	  task.copySurviveForkClassifications(child)
	  classifyNewTaskOnFork(child)
	  @lock.synchronize {	
	      @tasks[pid] = child
	  }
	  	  
	  notifyFork(child, task)
	  notifyNewTask(child)
	end
    end
    
    # Callback from on exec notification
    def onExec(pid, uid, name, isGuest, args=nil, envp=nil, rusage=nil)
        if ( !args ) then
                argsRequired = false
                @fullArgRequired.each { |pattern|
                    if ( name =~ pattern ) then
                        argsRequired = true
                       break 
                    end                            
                }
                
                return [DirectorNetlinkApi::REQUIRE_ARGS_AND_ENVP] if argsRequired
        end
        
        newTask, oldTask = registerTask(pid, uid, name, args, @nodeRepository.selfNode())
	oldTask.copySurviveExecClassifications(newTask) if oldTask
	classifyNewTaskOnExec(newTask)
        nil
    end
    
    # Callback on task exit
    def onExit(pid, exitCode, rusage)	
        deregisterTask(pid, exitCode)
    end
    
    # Callback in load balancer
    def onMigration(pid, response, rusage)
        # TODO: It is not guaranteed the migration will succeed at this time!
        return if !response
        
        # TODO: Ignoring migrate back decision
        return if response[0] != DirectorNetlinkApi::MIGRATE
        
        slotIndex = response[1]
        node = @membershipManager.coreManager.detachedNodes[slotIndex]
        node = @nodeRepository.selfNode if !node

        @lock.synchronize {
            task = @tasks[pid]
            if task
		oldNode = task.executionNode
                task.updateExecutionNode(node)
		notifyTaskNodeChanged(task, oldNode)
            end
        }        
    end
    
    def onMigratedHome(pid)	
	task = nil
        @lock.synchronize {
            task = @tasks[pid]	    
	}
	
	if task
	  oldNode = task.executionNode
	  task.updateExecutionNode(task.homeNode)	  
	  notifyTaskNodeChanged(task, oldNode)
	end
    end
    
    def onEmigrationFailed(pid)
	task = nil
        @lock.synchronize {
            task = @tasks[pid]	    
	}
	
	if task
	  oldNode = task.executionNode	  
	  task.updateExecutionNode(task.homeNode)	  
	  notifyTaskNodeChanged(task, oldNode)
	end      
    end
    
    def getTask(pid)
        task = nil
        @lock.synchronize {
            task = @tasks[pid]
        }              
	return task
    end
private    
    def notifyNewTask(task)
        @listeners.each { |listener| listener.newTask(task) }
    end

    def notifyFork(task, parentTask)
        @listeners.each { |listener| 	  
	    listener.taskFork(task, parentTask) if listener.respond_to?("taskFork")
	}
    end

    def notifyTaskNodeChanged(task, oldNode)
        @listeners.each { |listener| 	  	                  
	    listener.taskNodeChanged(task, oldNode) if listener.respond_to?("taskNodeChanged")
	}            
    end
    
    def notifyTaskExit(task, exitCode)
        @listeners.each { |listener| listener.taskExit(task, exitCode) }
    end
    
    def registerTask(pid, uid, name, args, homeNode=nil)
        alreadyContained = false
        task = TaskInfo.new(pid, uid, nil, name, args, homeNode)
	oldTask = nil
        @lock.synchronize {	
	    oldTask = @tasks[pid]
	    alreadyContained = true if oldTask
            @tasks[pid] = task
        }
        	
	# TODO: Notify listeners about exec, so that they know about change of code of the task?
        notifyNewTask(task) if !alreadyContained
	return [task, oldTask]
    end
    
    def deregisterTask(pid, exitCode)        
        task = nil
        @lock.synchronize {
            task = @tasks.delete(pid)
        }
        
        if ( task )
	   task.endTime = Time.now
           notifyTaskExit(task, exitCode)
        end        
    end    

    def classifyNewTaskOnExec(task)
      @execClassificators.each{ |classificator|
         classificator.classify(task) 
      }
    end    

    def classifyNewTaskOnFork(task)
      @forkClassificators.each{ |classificator|
         classificator.classify(task) 
      }
    end    
end
