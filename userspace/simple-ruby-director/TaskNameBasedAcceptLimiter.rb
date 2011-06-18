# This is a configurable accept limiter that does not accept any task if any task with matching any of restricted names is running
class TaskNameBasedAcceptLimiter        
    def initialize(names)
        @blockingPids = Set.new()
	@patterns = []
	names.each { |name|
	    @patterns << Regexp.new(name)
	}
	@inBlockingMode = false
    end
    
    # Returns maximum number of tasks that can be yet accepted (i.e. in addition to already accepted tasks)
    def maximumAcceptCount
	return 0 if @blockingPids.size > 0	
        return 100
    end

    # Callback from task repository
    def newTask(task)
       	@patterns.each { |pattern|
	    if ( task.name =~ pattern )
		blockingModeStarted(task) if @blockingPids.empty?
		@blockingPids.add(task.pid)
	        break
	    end
	}
    end
    
    # Callback from task repository
    def taskExit(task, exitCode)    
	emptyBefore = @blockingPids.empty?
	@blockingPids.delete(task.pid)
	blockingModeFinished(task) if !emptyBefore && @blockingPids.empty?
    end    
    
private
    def blockingModeStarted(task)
      $log.info("Task #{task.name}##{task.pid} blocked accepting of tasks")
      @inBlockingMode = true
    end
    
    def blockingModeFinished(task)
      $log.info("Task #{task.name}##{task.pid} unblocked accepting of tasks")
      @inBlockingMode = false
    end
    

    def migrateHomeThread
	# Wait 10 seconds before migrating home just to give locally running remote tasks some chance to finish
	Thread.sleep(10)
        while @inBlockingMode
	  
        end
    end

end