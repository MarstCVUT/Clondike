
# This class can be used instead of a real netlink connector for testing purposes
class MockNetlinkConnector
    def initialize (membershipManager)
            @membershipManager = membershipManager
            @npmHandlers = []
            @exitHandlers = []        
    end
    
    # Adds a new listener on exit events. Registered as a last listener on events
    def pushExitHandler(handler)
        @exitHandlers << handler;
    end

    # Adds a new listener on npm events. Registered as a last listener on events
    def pushNpmHandlers(handler)
        @npmHandlers << handler;
    end
        
    # Starts the processing thread, that listens on incoming messages from kernel
    def startProcessingThread
            @thread = Thread.new {
                loop do
                    sleep 50
                end
            }
    end

    # Waits till processing thread terminates
    def waitForProcessingThread
            @thread.join if @thread
    end
    
end