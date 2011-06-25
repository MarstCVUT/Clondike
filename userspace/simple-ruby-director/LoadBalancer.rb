require 'ConfigurablePatternMatcher'

#Makes the migration decisions (actual strategies are delegated to nested balancingStrategy instance)
#Non-preemptive decisions are made on exec time
#Preemptive are periodically evaluated (once per REBALANCING_INTERVAL seconds) and acted upon
class LoadBalancer
    REBALANCING_INTERVAL = 1
    
    def initialize(balancingStrategy, taskRepository, filesystemConnector)
        loadPatterns("migrateable.patterns")
        @balancingStrategy = balancingStrategy;
	@taskRepository = taskRepository
        # Listeners on migration decisions
        @migrationListeners = []
	@filesystemConnector = filesystemConnector
	
	ExceptionAwareThread.new() {
	    rebalancingThread()
	}    	
    end
    
    def registerMigrationListener(listener)
       @migrationListeners << listener
    end
    
    #Called when a new program is being "execv-ed"
    #Should return array with non-preemptive migration decisions and a target migman
    #in case a migration should be performed
    def onExec(pid, uid, name, is_guest, args=nil, envp=nil, rusage=nil)
        response = is_guest ? onExecGuest(pid, name, args, envp) : onExecCore(pid, uid, name, args, envp)
        notifyMigration(pid, response, rusage)
        response
    end
    
private
    include ConfigurablePatternMatcher
    
    def notifyMigration(pid, response, rusage)
        @migrationListeners.each { |listener| listener.onMigration(pid, response, rusage) }
    end
  
    # Returns true, if the binary specified by name can be migrated
    def canMigrate(name, uid)
        # TODO: This is incorrect.. it does not work for execution like "./binary"
	return false if !File.exists?(name)
        return matchesPattern(name)        
    end
    
    # Return slot index of the node where the process shall be migrated
    # Returns nil, if no migration should be performed    
    def getEmigrationTarget(pid, uid, name, args, envp)        
        #return nil if !canMigrate(name, uid)
	emigPreferred = isEmigrationPrefered(envp)
        return @balancingStrategy.findMigrationTarget(pid, uid, name, args, envp, emigPreferred);
    end
    
    def isEmigrationPrefered(envp)
      return envp.include?("EMIG=1")
    end
    
    def shouldMigrateBack(pid, name, args, envp)
        # TODO: Migration back not supported at the moment
        false
    end
    
    #Called, if the execv-ed task is a guest task
    def onExecGuest(pid, name, args, envp)        
        [shouldMigrateBack(pid,name, args, envp) ? DirectorNetlinkApi::MIGRATE_BACK : DirectorNetlinkApi::DO_NOT_MIGRATE]
    end
    
    #Called, if the execv-ed task is a normal task (which can be considered local
    #to a local core node)
    def onExecCore(pid, uid, name, args, envp)
	return [DirectorNetlinkApi::DO_NOT_MIGRATE] if ( !canMigrate(name, uid) );
	return [DirectorNetlinkApi::REQUIRE_ARGS_AND_ENVP] if ( !envp );
      
        migrationTarget = getEmigrationTarget(pid, uid, name, args, envp)
        if ( migrationTarget )
	  task = @taskRepository.getTask(pid)
	  if ( task )
	    $log.info("LoadBalancer decided to emigrate #{name}:#{pid} to node #{migrationTarget} (#{task.classifications_to_s})")
	  else
	    $log.warn("LoadBalancer cannot find info about task pid #{pid}")
	  end	  
          [DirectorNetlinkApi::MIGRATE, migrationTarget]
        else
	  task = @taskRepository.getTask(pid)
	  if ( task )
	    $log.info("LoadBalancer decided to keep #{name}:#{pid} home (#{task.classifications_to_s})")
	  else
	    $log.warn("LoadBalancer cannot find info about task pid #{pid} (kept home)")
	  end
          [DirectorNetlinkApi::DO_NOT_MIGRATE]
        end
    end
    
    # Periodically check with strategies if a rebelance is required
    def rebalancingThread()
	while true do
	  sleep(REBALANCING_INTERVAL)
	
	  rebalancingPlan = @balancingStrategy.findRebalancing
	  next if !rebalancingPlan
	  
	  rebalancingPlan.each { |pid, nodeIndex|
	      $log.info("LoadBalancer decided migrate preemptively #{pid} to #{nodeIndex}")
	      @filesystemConnector.emigratePreemptively(nodeIndex, pid)
	  }
	end    
    end    
end
