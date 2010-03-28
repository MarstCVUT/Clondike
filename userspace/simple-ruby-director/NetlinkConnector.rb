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
		@userMessageHandlers = []
                begin
                    DirectorNetlinkApi.instance.registerNpmCallback(self, :connetorNpmCallbackFunction)
                    DirectorNetlinkApi.instance.registerNpmFullCallback(self, :connectorNpmFullCallbackFunction)
                    DirectorNetlinkApi.instance.registerNodeConnectedCallback(self, :connectorNodeConnectedCallbackFunction)
                    DirectorNetlinkApi.instance.registerTaskExittedCallback(self, :connectorTaskExittedCallbackFunction)
                    DirectorNetlinkApi.instance.registerImmigrateRequestCallback(self, :connectorImmigrationRequestCallbackFunction)
		    DirectorNetlinkApi.instance.registerUserMessageReceivedCallback(self, :connectorUserMessageReceivedCallbackFunction)
                rescue
                    raise "Failed to initialize netlink API!!!"
                end
	end

        # Adds a new listener on npm events. Registered as a last listener on events
        def pushNpmHandlers(handler)
            @npmHandlers << handler;
        end
                
	def connetorNpmCallbackFunction (pid, uid, name, is_guest)
                result = nil
                @npmHandlers.each do |handler|
                    result = handler.onExec(pid, uid, name, is_guest)
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
        
        def connectorImmigrationRequestCallbackFunction(uid, slotIndex, name)
                result = nil
                puts "Immigration request for process #{name} "
                true
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

        def pushUserMessageHandler(handler)
            @userMessageHandlers << handler;
        end

	def connectorUserMessageReceivedCallbackFunction(slotType, slotIndex, messageLength, message)
		$log.info("Received user message on slot: #{slotType}/#{slotIndex} of length #{messageLength}")

                @userMessageHandlers.each do |handler|
                    handler.userMessageReceived(ManagerSlot.new(slotType, slotIndex), messageLength, message)
                end
	end

	def connectorSendUserMessage(managerSlot, messageLength, message)
		$log.debug( "Sending user message to slot #{managerSlot}")
		DirectorNetlinkApi.instance.sendUserMessage(managerSlot.slotType, managerSlot.slotIndex, messageLength, message)
	end

        # Adds a new listener on exit events. Registered as a last listener on events
        def pushExitHandler(handler)
            @exitHandlers << handler;
        end
        
        def connectorTaskExittedCallbackFunction(pid, exitCode)
            #puts "Pid #{pid} exitted with code #{exitCode}"
                @exitHandlers.each do |handler|
                    handler.onExit(pid, exitCode)
                end
            
        end

	# Starts the processing thread, that listens on incoming messages from kernel
	def startProcessingThread
		@thread = Thread.new {
                    DirectorNetlinkApi.instance.runProcessingLoop
                }
	end

	# Waits till processing thread terminates
	def waitForProcessingThread
		@thread.join if @thread
	end
end