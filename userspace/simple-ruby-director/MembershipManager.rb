require 'Manager'

#This class keeps internal both core and detached node managers. Via these manager
#it keeps track about membership info about all cluster in which this machine
#participates
#
#Initially, it parses membership information from file
#Later, it responds to various external events, to keep membership information
#up to date
#
# This class is repsonsible for connecting new nodes (or disconnecting existing if they are no longer required).
# When some other class wants to connect/disconnect remote nodes (for example LoadBalancer), it should tell this class
class MembershipManager
	# Manager of the core node
	attr_reader :coreManager
	# Array of managers of all detached nodes on this node (every connection to remote CCN has its own dedicated datached manager)
        # Detached managers are placed in the array in the slot corresponding to their
        # slot index in kernel module
        # Slots with no manager are nil
	attr_reader :detachedManagers
	
	# How many peers should manager try to keep connected even if there are no explicit peers requirements from higher layers.
	attr_accessor :minimumConnectedPeers
    
        def initialize(filesystemConnector, nodeRepository, trustManagement)
            @filesystemConnector = filesystemConnector
            @nodeRepository = nodeRepository
	    @trustManagement = trustManagement
	    @minimumConnectedPeers = 50

            # Init core manager reference, if there is a core node registered on this machine
            @coreManager = FilesystemNodeBuilder.new().parseCoreManager(@filesystemConnector, @nodeRepository);

            # Check via fs api if there is dn registered
            @detachedManagers = FilesystemNodeBuilder.new().parseDetachedManagers(@filesystemConnector, @nodeRepository);            	    	    
	    
	    startAutoconnectingThread()
        end
        
        #This callback is invoked, when we learn about existence of a new node
        def notifyNewNode(node)
            #Dummy.. we simply try to connect to every new node we learn about
            connectToNode(node)
        end
        
        def canConnect(authenticationData)
            verifyResult = @trustManagement.verifyAuthentication(authenticationData)
            #puts "A #{verifyResult} B #{verifyResult != nil}"
            return verifyResult != nil
        end
        
        #This callback is invoked, when a new remote node has connected to our core node
        def nodeConnected(address, slotIndex)
            #nodeId = @filesystemConnector.findNodeIdByAddress(address)
            #node, isNew = @nodeRepository.getOrCreateNode(nodeId, address)                
	    placeHolderNode = Node.new(nil, address) # Just a placeholder node with no id, not even registered to repository
            $log.debug("Adding detached placeholder node: #{slotIndex} .. #{placeHolderNode}")
            @coreManager.registerDetachedNode(slotIndex, placeHolderNode)
        end
	
        #This callback is invoked, when remote node is disconnected (could be both core or detached remote node)
        def nodeDisconnected(managerSlot)
            $log.debug("Node disconnected: #{managerSlot}")
	    if ( managerSlot.slotType == DETACHED_MANAGER_SLOT )
	      $log.warn("Slot #{managerSlot.slotIndex} is already empty!") if !@detachedManagers[managerSlot.slotIndex]
	      @detachedManagers[managerSlot.slotIndex] = nil
	    else
	      if ( !@coreManager.detachedNodes[managerSlot.slotIndex] )
		$log.warn("Core manager slot #{managerSlot.slotIndex} is already empty!")
		return
	      end
		      
	      @coreManager.unregisterDetachedNode(managerSlot.slotIndex)
	    end            
        end	

        def connectToNode(node)
            # The connection attempt is done in a separate thread so that we do not block receiving
            # Better would be to make connection non-blocking (especially the auth-negotiation which actually uses message interconnect)
            ExceptionAwareThread.new() {
                nodeIpAddress = node.ipAddress                
		nodePublicKey = @trustManagement.getKey(node.id);
		
		# Do not know the key yet
		if nodePublicKey then
		  $log.debug("Trying to connect to #{nodeIpAddress}")
		  session = @trustManagement.authenticate(node.id, nodePublicKey)
		  
		  if session then
		      # TODO: Devel proof
		      succeeded = @filesystemConnector.connect(nodeIpAddress, session.authenticationProof)
		      $log.info("Connection attempt to #{nodeIpAddress} with proof #{session.authenticationProof} #{succeeded ? 'succeeded' : 'failed'}.")
		      if succeeded
			  slotIndex = @filesystemConnector.findDetachedManagerSlot(nodeIpAddress)
			  @detachedManagers[slotIndex] = DetachedNodeManager.new(node, slotIndex)
		      end
		  end
		end
            }
        end

private
  def containsDetachedNode(node)
    res = false;
    @detachedManagers.each { |manager|
       if ( manager != nil && manager.coreNode == node )
          res = true                    
	  break
       end
    }
    return res;
  end

  def startAutoconnectingThread()
    ExceptionAwareThread.new() {
      while true do
	sleep 10
	connectAllUnconnectedNodes()	
      end
    }
  end

  def connectAllUnconnectedNodes()
     @nodeRepository.eachNode { |node|
          # TODO: This is incorrect, we just connect all nodes not yet connected
          # A better solution would be to send a "connect-me" message to peer if connectedNodesCount < minimumConnectedPeers .. this way, we would act from a position of core node, instead of detached node as now..
	  connectToNode(node) if !containsDetachedNode(node)
     #    connectToNode(node) if @coreManager.connectedNodesCount < @minimumConnectedPeers && !@coreManager.containsNode(node)
     }
  end
end