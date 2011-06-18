# Accept limiters generally can limit a number of external tasks this node is willing to acccept
# TODO: Limiters are not used when accepting tasks, so we can in fact accept more taks. We need to add support for REJECTING tasks!
# 
# This is a configurable accept limiter that does not accept any task if any task with matching any of restricted names is running
class TaskNameBasedAcceptLimiter        
    def initialize(names)
        @blockingPids = Set.new()
	@patterns = []
	names.each { |name|
	    @patterns << Regexp.new(name)
	}
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
		$log.info("Task #{task.name}##{task.pid} blocked accepting of tasks") if @blockingPids.empty?
		@blockingPids.add(task.pid)
	        break
	    end
	}
    end
    
    # Callback from task repository
    def taskExit(task, exitCode)    
	emptyBefore = @blockingPids.empty?
	@blockingPids.delete(task.pid)
	$log.info("Task #{task.name}##{task.pid} unblocked accepting of tasks") if !emptyBefore && @blockingPids.empty?
    end    
end