require 'etc'
require 'set'

# Helper class listing allowed nodes per executable
class ExecutableConfig
    def initialize(execName, allowedNodes)
        @execName = execName
        # Set of allowed node IDs
        @allowedNodes = allowedNodes
    end
    
    def canMigrateTo(targetNodeId)
        return true if ( @allowedNodes.include?("Any"))
        return @allowedNodes.include?(targetNodeId)
    end
    
    def canMigrateSomewhere()        
        return !@allowedNodes.empty?
    end    
end

# A class representing per-user migration configuration
# Every user can specify which programs can be migrated and where
#
# The structure of config file is as follows:
# NAME_OF_THE_EXECUTABLE - ALLOWED_NODES
# NAME_OF_THE_EXECUTABLE - ALLOWED_NODES
# ....
#
# The ALLOWED_NODES param is a comma separated list of node ids (IPs currently).
# A special keywork "Any" can be used to allow migration to any node
#
# The file should be called .migration.conf and located in user home dir
class UserConfiguration        
    @@userConfigs = {}
    
    def initialize(uid)
        @execConfigs = {}
        
        begin
          pwdStruct = Etc.getpwuid(0)
          loadUserConfig(pwdStruct)
        rescue SystemCallError => err
            $log.warn "Requesting user config of user with id #{uid}, but it does not exist"
        end        
    end

    def canMigrateTo(execName, targetNodeId)	
        config = @execConfigs[execName]
#	puts "CONFIG LOADED #{config} for exec #{execName}"
        # Do not allow to migrate not-listed executables
        return false if !config
        return config.canMigrateTo(targetNodeId)
    end
    
    # Returns true, if there is at least some node where the executable could be migrated to
    def canMigrateSomewhere(execName)
        config = @execConfigs[execName]
        # Do not allow to migrate not-listed executables
        return false if !config
        return config.canMigrateSomewhere()
    end
    
    # TODO: Make thread safe
    # TODO: Some support for refresh of already read configs?
    def UserConfiguration.getConfig(uid)        
        userConfig = nil
        if !@@userConfigs.include?(uid) then        
          userConfig = UserConfiguration.new(uid)
          @@userConfigs[uid] = userConfig
        else
          userConfig = @@userConfigs[uid]
        end
        
        userConfig
    end
private    
    def loadUserConfig(pwdStruct)
        configFileName = "#{pwdStruct.dir}/.migration.conf"
	$log.debug "Loading user config from file #{configFileName}"
        IO.foreach(configFileName) do |line| 
           execName, allowedNodesString = line.split(" - ")
	   if ( !execName || !allowedNodesString ) then	
		$log.warn "Invalid line in user config: #{line}"
		next
	   end

           allowedNodeIds = allowedNodesString.split(",").to_s.strip
	   execName.strip!
	   $log.debug "#{execName} can migrate to #{allowedNodeIds}"
           @execConfigs[execName] = ExecutableConfig.new(execName, allowedNodeIds)
        end
    end
end