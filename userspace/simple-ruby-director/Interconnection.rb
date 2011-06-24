require 'PersistentIdSequence.rb'
require 'trust/TrustManagement.rb'
require 'Util.rb'

# Class for handling inter node message exchanges
# TODO: This is now a duplicite mechanism with InformationDistributionStrategy -> Rewrite that class to use this class?
class Interconnection
    def initialize(messageDispatcher, configDirectory)
        # Handlers of message receiving
        @handlers = {}
        @messageDispatcher = messageDispatcher        
        @sequenceId = PersistentIdSequence.new("#{configDirectory}/msg-seq")
        @ackTracking = AckTracking.new(:doDispatch, configDirectory)
        
        # Default messages are not acked
        @defaultDeliveryOptions = DeliveryOptions::NO_ACK
	# FIFO queue of received messages to be processed
	@messageQueue = BlockingQueue.new
    end
    
    def start(trustManagement, netlinkConnector, identityProvider)
      	@identityProvider = identityProvider
	@netlinkConnector = netlinkConnector
        @trustManagement = trustManagement
        @recvThread = Thread.new() { recvMessageThread()  }
	@processThread = ExceptionAwareThread.new() { processMessageThread() }
    end
    
    # Schedules message to be sent
    # TO: Either null (broadcast) or a nodeId (publicKey) of the target node
    # MESSAGE: Message object to be sent
    def dispatch(to, message, deliveryOptions = nil)
        deliveryOptions = @defaultDeliveryOptions if !deliveryOptions
        messageId = @sequenceId.nextId
        messageWrapper = MessageWrapper.new(messageId, to, message, deliveryOptions.requireAck)
        @ackTracking.newMessageSent(messageWrapper, deliveryOptions)        
        doDispatch(to, messageWrapper)
    end

    def dispatchToSlot(slot, message)
	serializedMessage = Marshal.dump(message)
	# No strategies for slot messages, they simply go through the kernel now
	@netlinkConnector.connectorSendUserMessage(slot, serializedMessage.length, serializedMessage)
    end
    
    # Registers receive message handler for a particular message class
    def addReceiveHandler(messageClass, handler)
        handlerSet = @handlers[messageClass]
        if ( !handlerSet )
            handlerSet = Set.new
            @handlers[messageClass] = handlerSet
        end
        
        handlerSet.add(handler)
    end
    
    # Callback function to handle incoming user messages (registered to netlink API)
    def userMessageReceived(managerSlot, messageLength, message)
	deserializedMessage = Marshal.load(message)
	@messageQueue.enqueue([deserializedMessage, managerSlot])
    end
private
    def recvMessageThread
        while true
            begin
               message = @messageDispatcher.receive()                                              		
               handle(message)
            rescue => err
                $log.error "Error in recvMessageThread: #{err.message} \n#{err.backtrace.join("\n")}"
            end
        end
    end
    
    # Async thread processing unwrapped messages
    def processMessageThread
	while true
	    message, sender = @messageQueue.dequeue
	    handleUnwrappedMessage(message, sender)
	end
    end
    
    def handle(message)
        case message
            when MessageWrapper                
                currentId = @identityProvider.getCurrentId
                # Ignore non-broadcast message that are not sent for us
                return if (message.target != nil && message.target != currentId)
                                
                doDispatch(message.target, AckMessage.new(message.messageId, currentId, @trustManagement)) if message.requiresAck
		# TODO: Extend wrapper by key of sender and pass the key here
		@messageQueue.enqueue([message.message, nil])
            when AckMessage
                @ackTracking.ackReceived(message.messageId, message.recipientId) if ( message.verifySignature(@trustManagement))
        else
            $log.error "Unexpected message class arrived -> #{message.class}"
        end        
    end
    
    def handleUnwrappedMessage(message, from)
        handlerSet = @handlers[message.class]
        return if !handlerSet
        
        handlerSet.each { |handler|
	    if ( handler.respond_to?("handleFrom") )
		    # Handle from version has additional parameter that can be id NodeId or ManagerSlot, depending on a way the message arrived
		    handler.handleFrom(message, from)
	    else
	            handler.handle(message)
	    end
        }        
    end
    
    # Internal implementation of dispatch. It does not care of acks, it is assumed to be handled in public method
    def doDispatch(to, message)
        @messageDispatcher.dispatch(to, message)
    end    
end

# Simple strategy for dispatching message.. broadcast everything
class InterconnectionUDPMessageDispatcher
    DEFAULT_PORT = 5387
    
    def initialize(port = DEFAULT_PORT)
       @socket = UDPSocket.new       
       @socket.bind("", port)
       @port = port
       @socket.setsockopt(Socket::SOL_SOCKET, Socket::SO_BROADCAST, 1)        
    end
    
    # Schedules message to be sent
    # TO: Either null (broadcast) or a nodeId (publicKey) of the target node
    # MESSAGE: Message object to be sent
    def dispatch(to, message)
#        $log.debug("Interconnect is dispatching message to #{to}.")
        # Dummy implementation -> Broadcast everything
        @socket.send(Marshal.dump(message), 0, "255.255.255.255", @port)
    end
    
    def receive()
          # Loop while we get a message from remote IP (has to ignore local messages)
          while (true) do
            recvData, addr = @socket.recvfrom(60000)            
            break if ( !isLocalIp(addr))               
            #$log.debug("Ignoring message from local IP (#{addr})")
          end
          message = Marshal.load(recvData)        
 #         $log.debug("Interconnect received message.")
          return message
    end
end

# Class configuring delivery options of a message
class DeliveryOptions            
    # Boolean flag whether the ack is required 
    attr_accessor :requireAck
    # Maximum time (in seconds) to wait till we discard this message altogether
    # nil if no timeout applies
    attr_accessor :maximumTimeout
    # Seconds between retransmits
    attr_accessor :retransmitInterval
    
    def initialize(requireAck, maximumTimeout, retransmitInterval)
        @requireAck = requireAck
        @maximumTimeout = maximumTimeout
        @retransmitInterval = retransmitInterval
    end

    NO_ACK = DeliveryOptions.new(false, 0, 0)
    # Wait at most 10 days for ack, retransmit every hour
    ACK_10_DAYS = DeliveryOptions.new(true, 10*24*60*60, 60*60)
    # Wait at most one hour for ack, retransmit every minute
    ACK_1_HOUR = DeliveryOptions.new(true, 60*60, 60)
    # Wait at most one minute for ack, retransmit every 5 seconds
    ACK_1_MIN = DeliveryOptions.new(true, 60, 5)    
end

# Class wrapping a generic message and giving it a unique tracking number
class MessageWrapper
    # Unique message ID
    attr_reader :messageId
    # Public key of the target recipient (nil in case of broadcast)
    attr_reader :target
    # Original message
    attr_reader :message
    # Flag, indicating if we wait for ack (hint to recipient)
    attr_reader :requiresAck
    
    def initialize(messageId, target, message, requiresAck)
        @messageId = messageId
        @target = target
        @message = message
        @requiresAck = requiresAck
    end
end

# Tracking of message status
class MessageStatus    
    attr_reader :messageWrapper
    attr_reader :firstTimeSent
    attr_accessor :lastTimeSent
    attr_reader :deliveryOptions
    
    def initialize(messageWrapper, firstTimeSent, deliveryOptions)
        @messageWrapper = messageWrapper
        @firstTimeSent = firstTimeSent
        @lastTimeSent = firstTimeSent
        @deliveryOptions = deliveryOptions
    end
    
    def expireTime
        return nil if @deliveryOptions.maximumTimeout == nil
        
        return firstTimeSent + @deliveryOptions.maximumTimeout
    end
    
    def isObsolote(now)
        expireTime = expireTime()
        
        return expireTime != nil || expireTime < now ? true : false
    end
    
    def nextRetransmitTime()
        return @lastTimeSent + @deliveryOptions.retransmitInterval
    end
    
    def shouldRetransmitNow(now)
        return now > nextRetransmitTime()
    end
end

# Class that actually keeps track of pending acks, issues restransmits and deletes old pending acks
class AckTracking
    def initialize(retransmitMethod, directory)
        # Method to be called for message retransmission
        @retransmitMethod = retransmitMethod
        # Hash of pending acks
        # Key is the message ID, value is the MessageStatus class
        @pendingAcks = {}
        # Guard of pendingAcks
        @lock = Mutex.new
        # Root directory where the data are stored
        @saveDirectory = "#{directory}/acks"
        createAckStoreDir()
        loadAllAckData()
        
        Thread.new() {
            while true do
                sleep(0.25) # 250ms sleep
                cleanObsoloteRecords()
                retransmitMessages()
            end
        }
    end
    
    def ackReceived(messageId, recipient)
        $log.debug("Ack received for message ID: #{messageId}")                
        @lock.synchronize  {
            messageStatus = @pendingAcks[messageId]
            return if !messageStatus
            return if messageStatus.messageWrapper.target != recipient
            
            @pendingAcks.delete(messageId)            
        }        
        deleteAckData(messageId)
    end
    
    def newMessageSent(messageWrapper, deliveryOptions)
        return if !deliveryOptions.requireAck
        
        messageStatus = nil
        @lock.synchronize {
            if @pendingAcks.member?(messageWrapper.messageId) then
                $log.warn("Duplicate message id in AckTracking! Id: #{messageWrapper.messageId}")
                return
            end
            
            messageStatus = MessageStatus.new(messageWrapper, Time.now, deliveryOptions)
            @pendingAcks[messageWrapper.messageId] = messageStatus
        }
        
        saveAckData(messageStatus)
    end
    
private
    def cleanObsoloteRecords
        now = Time.now
        @pendingAcks.each { |key, value|  
            if ( value.isObsolote(now)) then
                @pendingAcks.delete(key)
                deleteAckData(key)
            end
        }        
    end
    
    def retransmitMessages
        now = Time.now
        @pendingAcks.each { |key, value|  
            messageStatus = value
            if ( messageStatus.shouldRetransmitNow(now)) then
                $log.info("Retransmitting message ID: #{messageStatus.messageWrapper.messageId}")                
                @retransmitMethod.call(messageStatus.messageWrapper.target, messageStatus.messageWrapper.message)
                messageStatus.lastTimeSent = Time.now
                saveAckData(messageStatus)
            end
        }
    end
    
    def createAckStoreDir
        FileUtils::mkdir_p(@saveDirectory) if ( !File.exists?(@saveDirectory))
    end
    
    def saveAckData(messageStatus)
        fileName = "#{@saveDirectory}/#{messageStatus.messageWrapper.messageId}"
        File.open(fileName, "w") { |f| Marshal.dump(messageStatus, f) }
    end
    
    def deleteAckData(messageId)
        fileName = "#{@saveDirectory}/#{messageId}"
        File::delete(fileName)
    end
    
    def loadAllAckData()
      Find.find(@saveDirectory) do |path|
          if File.basename(path).match(/\d/)
              open(path) { |f| 
                  messageStatus = Marshal.load(f) 
                  @pendingAcks[messageStatus.messageWrapper.messageId] = messageStatus
              }
          end        
      end
    end
end

# Message for acking of message delivery
class AckMessage
   # Id of acked message
   attr_reader :messageId
   # Timestamp when the ack was generated
   attr_reader :timestamp
   # Signature confiruming valid ack
   attr_reader :signature
   # Id of node that recieved the message
   attr_reader :recipientId

   def initialize(messageId, recipientId, trustManagement)
       @timestamp = Time.now
       @recipientId = recipientId
       @messageId = messageId
       sign(trustManagement)
   end      
   
   def verifySignature(trustManagement)
       return false if (!signature)
       recipientKey = trustManagement.getKey(@recipientId)
       if !recipientKey then
	 $log.warn("Cannot verify signature, recipient key uknown: #{@recipientId}")
	 return false
       end
       
       trustManagement.verifySignature(dataToSign(), @signature, recipientKey)
   end
private
   def dataToSign
       "T: #{@timestamp} MID: #{@messageId}"
   end
   
   def sign(trustManagement)
       @signature = trustManagement.sign(dataToSign())
   end
end
