require "directorApi"

# Class, that interacts with the kernel netlink api
#
# Npm callbacks are processed by registered handlers. If first handler does not
# return any value, we pass the request to the next... if not handler returns
# any result DO_NOT_MIGRATE is returned
class NetlinkConnector
	def initialize (membershipManager)
		@membershipManager = membershipManager
                @npmHandlers = []
                @exitHandlers = []
		@forkHandlers = []
		@userMessageHandlers = []
		@immigrationHandlers = []
		@immigrationConfirmedHandlers = []
		@migrationFailedHandlers = []
		@migratedHomeHandlers = []
	end
	
	# Registers a netlink connector instance to native handler
	def self.register(instance)
                begin
                    DirectorNetlinkApi.instance.registerNpmCallback(instance, :connectorNpmCallbackFunction)
                    DirectorNetlinkApi.instance.registerNpmFullCallback(instance, :connectorNpmFullCallbackFunction)
                    DirectorNetlinkApi.instance.registerNodeConnectedCallback(instance, :connectorNodeConnectedCallbackFunction)
		    DirectorNetlinkApi.instance.registerNodeDisconnectedCallback(instance, :connectorNodeDisconnectedCallbackFunction)
                    DirectorNetlinkApi.instance.registerTaskExittedCallback(instance, :connectorTaskExittedCallbackFunction)
		    DirectorNetlinkApi.instance.registerTaskForkedCallback(instance, :connectorTaskForkedCallbackFunction)
                    DirectorNetlinkApi.instance.registerImmigrateRequestCallback(instance, :connectorImmigrationRequestCallbackFunction)
		    DirectorNetlinkApi.instance.registerImmigrationConfirmedCallback(instance, :connectorImmigrationConfirmedCallbackFunction)
		    DirectorNetlinkApi.instance.registerEmigrationFailedCallback(instance, :connectorEmigrationFailedCallbackFunction)
		    DirectorNetlinkApi.instance.registerMigratedHomeCallback(instance, :connectorMigratedHomeCallbackFunction)
		    DirectorNetlinkApi.instance.registerUserMessageReceivedCallback(instance, :connectorUserMessageReceivedCallbackFunction)
                rescue => err
		    puts "#{err.backtrace.join("\n")}"		    
                    raise "Failed to initialize netlink API!!!"		    
                end	  
	end

        # Adds a new listener on npm events. Registered as a last listener on events
        def pushNpmHandlers(handler)
            @npmHandlers << handler;
        end
                
	def connectorNpmCallbackFunction (pid, uid, name, is_guest, rusage)
                result = nil
                @npmHandlers.each do |handler|
                    result = handler.onExec(pid, uid, name, is_guest, nil, nil, rusage)
                    break if result
                end 
                result = [DirectorNetlinkApi::DO_NOT_MIGRATE] if !result
		$log.info("(Result #{result[0]} target #{result[1]}) for #{name}") if ( result[0] == DirectorNetlinkApi::MIGRATE_BACK || result[0] == DirectorNetlinkApi::MIGRATE)
                result                
	end

	def connectorNpmFullCallbackFunction (pid, uid, name, is_guest, args, envp)            
                result = nil
                @npmHandlers.each do |handler|
                    result = handler.onExec(pid, uid, name, is_guest, args, envp)
                    break if result
                end
                result = [DirectorNetlinkApi::DO_NOT_MIGRATE] if !result
		$log.info("#{result[0]} for #{name}") if ( result[0] == DirectorNetlinkApi::MIGRATE_BACK || result[0] == DirectorNetlinkApi::MIGRATE)
                result
	end
        
        def pushImmigrationHandler(handler)
            @immigrationHandlers << handler;
        end	
	
        def connectorImmigrationRequestCallbackFunction(uid, slotIndex, name)                
                $log.info("Immigration request for process #{name}")
		result = true
		@immigrationHandlers.each do |handler|
		  node = @membershipManager.detachedManagers[slotIndex].coreNode
		  result = result && handler.onImmigrationRequest(node, name)
		  break if !result
		end
		
		$log.info("Immigration request for process #{name} REJECTED!") if !result
		
                return result
        end

	def pushImmigrationConfirmedHandler(handler)
            @immigrationConfirmedHandlers << handler;
        end	
	
        def connectorImmigrationConfirmedCallbackFunction(uid, slotIndex, name, localPid, remotePid)
                #$log.info("Immigration of process #{name} (#{localPid}, #{remotePid}) confirmed.")
		@immigrationConfirmedHandlers.each do |handler|
		    node = @membershipManager.detachedManagers[slotIndex].coreNode
		    handler.onImmigrationConfirmed(node, name, localPid, remotePid)
		end
        end

	def connectorNodeConnectedCallbackFunction (address, slotIndex, authenticationData)
                $log.info("New node tries to connect: #{address} - #{slotIndex} - #{authenticationData}")
                address = address.split(":").first  # Remove port section of address
		if ( @membershipManager.coreManager and @membershipManager.canConnect(authenticationData)) then
                        puts "Accept"
                        @membershipManager.nodeConnected(address, slotIndex)
                        return true
		else
                        puts "Reject"
			return false
                end
	end

	def connectorNodeDisconnectedCallbackFunction (slotIndex, slotType, reason)
		# Reason: 0 locally requested, 1 remotely requested... not imporant right now..
                $log.info("Node disconnected: #{slotType}/#{slotIndex} Reason: #{reason}")
		
		@membershipManager.nodeDisconnected(ManagerSlot.new(slotType, slotIndex))
	end

        def pushUserMessageHandler(handler)
            @userMessageHandlers << handler;
        end

	def connectorUserMessageReceivedCallbackFunction(slotType, slotIndex, messageLength, message)
#		$log.info("Received user message on slot: #{slotType}/#{slotIndex} of length #{messageLength}")

	    @userMessageHandlers.each do |handler|
		handler.userMessageReceived(ManagerSlot.new(slotType, slotIndex), messageLength, message)
	    end
	end

	def connectorSendUserMessage(managerSlot, messageLength, message)
#		$log.debug( "Sending user message to slot #{managerSlot}")
	    DirectorNetlinkApi.instance.sendUserMessage(managerSlot.slotType, managerSlot.slotIndex, messageLength, message)
	end

        # Adds a new listener on exit events. Registered as a last listener on events
        def pushExitHandler(handler)
            @exitHandlers << handler;
        end
        
        def connectorTaskExittedCallbackFunction(pid, exitCode, rusage)
            #puts "Pid #{pid} exitted with code #{exitCode}"
	    @exitHandlers.each do |handler|
		handler.onExit(pid, exitCode, rusage)
	    end            
        end

        def pushForkHandler(handler)
            @forkHandlers << handler;
        end

	def connectorTaskForkedCallbackFunction(pid, parentPid)
	    @forkHandlers.each do |handler|
		handler.onFork(pid, parentPid)
	    end            	  
        end

        def pushEmigrationFailedHandler(handler)
            @migrationFailedHandlers << handler;
        end

	def connectorEmigrationFailedCallbackFunction(pid)
	    $log.info("Emigration failed for pid #{pid}")
	    
	    @migrationFailedHandlers.each do |handler|
		handler.onEmigrationFailed(pid)
	    end
	end
	
        def pushMigratedHomeHandler(handler)
            @migratedHomeHandlers << handler;
        end
	
	def connectorMigratedHomeCallbackFunction(pid)
	    $log.info("Migrated home: #{pid}")
	    
	    @migratedHomeHandlers.each do |handler|
		handler.onMigratedHome(pid)
	    end
	end	

	# Starts the processing thread, that listens on incoming messages from kernel
	def startProcessingThread
		@thread = ExceptionAwareThread.new {
                    DirectorNetlinkApi.instance.runProcessingLoop
                }
	end

	# Waits till processing thread terminates
	def waitForProcessingThread
		@thread.join if @thread
	end
end
