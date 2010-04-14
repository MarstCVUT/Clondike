require 'ConfigurablePatternMatcher'

#Makes the migration decisions.. currently only non-preemptive
class LoadBalancer
    def initialize(balancingStrategy)
        loadPatterns("migrateable.patterns")
        @balancingStrategy = balancingStrategy;
        # Listeners on migration decisions
        @migrationListeners = []
    end
    
    def registerMigrationListener(listener)
       @migrationListeners << listener
    end
    
    #Called when a new program is being "execv-ed"
    #Should return array with non-preemptive migration decisions and a target migman
    #in case a migration should be performed
    def onExec(pid, uid, name, is_guest, args=nil, envp=nil)
        response = is_guest ? onExecGuest(pid, name, args, envp) : onExecCore(pid, uid, name, args, envp)
        notifyMigration(pid, response)
        response
    end
    
private
    include ConfigurablePatternMatcher
    
    def notifyMigration(pid, response)
        @migrationListeners.each { |listener| listener.onMigration(pid, response) }
    end
  
    # Returns true, if the binary specified by name can be migrated
    def canMigrate(name, uid)
        # TODO: This is incorrect.. it does not work for execution like "./binary"
	return false if !File.exists?(name)
        #return true if matchesPattern(name)
        true
    end
    
    # Return slot index of the node where the process shall be migrated
    # Returns nil, if no migration should be performed    
    def getEmigrationTarget(pid, uid, name, args, envp)        
        return nil if !canMigrate(name, uid)
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
          [DirectorNetlinkApi::MIGRATE, migrationTarget]
        else
          [DirectorNetlinkApi::DO_NOT_MIGRATE]
        end
    end
end