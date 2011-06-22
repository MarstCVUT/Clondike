require 'BlockingQueue'
require 'socket'

#This class handles propagation and receiving of NodeInfos
class InformationDistributionStrategy
    #UDP port used for information exchange
    PORT = 54433
    
    def initialize(nodeInfoProvider, informationConsumer)
       # Disabling of socket reverse lookups saves us some potential nasty jitter on recvfrom, where the reverse lookup would be performed
       BasicSocket.do_not_reverse_lookup = true
       @nodeInfoProvider = nodeInfoProvider
       @informationConsumer = informationConsumer
       @sendQueue = BlockingQueue.new()
       @socket = UDPSocket.new
       @socket.bind("", PORT)
       @socket.setsockopt(Socket::SOL_SOCKET, Socket::SO_BROADCAST, 1)
    end
    
    def start
        @extractThread = Thread.new() { nodeInfoExtractThread() }
        @sendThread = Thread.new() { sendInfoThread() }
        @recvThread = Thread.new() { recvInfoThread()  }
    end
    
    def waitForFinished
        @extractThread.join
        @sendThread.join
        @recvThread.join
    end
   
    #Callback method, informing about significant change in a dynamic node info
    def notifyChange(nodeInfo)
        @sendQueue.enqueue(nodeInfo)
    end
private
    #Enqueues dynamic info about the current node to be send (once per second)
    def nodeInfoExtractThread
        while true            
            sleep(1)
            @sendQueue.enqueue(@nodeInfoProvider.getCurrentInfoWithId)          
        end
    end
    
    def sendInfoThread
        while true
            sendElement = @sendQueue.dequeue()
            @socket.send(Marshal.dump(sendElement), 0, "255.255.255.255", PORT)
        end
    end
    
    def recvInfoThread
        while true
            begin
              recvData, addr = @socket.recvfrom(60000)
#	      timedExecution("LS") {
		recvElement = Marshal.load(recvData)
		@informationConsumer.infoReceived(addr[2], recvElement) if @informationConsumer
#	      }
              #puts "Received from #{addr[2]} Element: #{recvElement}"
            rescue => err
                $log.error "Error in recvInfoThread #{err.backtrace.join("\n")}"                
            end
        end
    end
end