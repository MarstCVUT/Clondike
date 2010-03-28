require 'Manager'

#This class keeps internal both core and detached node managers. Via these manager
#it keeps track about membership info about all cluster in which this machine
#participates
#
#Initially, it parses membership information from file
#Later, it responds to various external events, to keep membership information
#up to date
class MembershipManager
	# Manager of the core node
	attr_reader :coreManager
	# Array of managers of all detached nodes on this node (every connection to remote CCN has its own dedicated datached manager)
        # Detached managers are placed in the array in the slot corresponding to their
        # slot index in kernel module
        # Slots with no manager are nil
	attr_reader :detachedManagers
    
        def initialize(filesystemConnector, nodeRepository, trustManagement)
            @filesystemConnector = filesystemConnector
            @nodeRepository = nodeRepository
	    @trustManagement = trustManagement

            # Init core manager reference, if there is a core node registered on this machine
            @coreManager = FilesystemNodeBuilder.new().parseCoreManager(@filesystemConnector, @nodeRepository);

            # Check via fs api if there is dn registered
            @detachedManagers = FilesystemNodeBuilder.new().parseDetachedManagers(@filesystemConnector, @nodeRepository);            
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
            nodeId = @filesystemConnector.findNodeIdByAddress(address)
            node, isNew = @nodeRepository.getOrCreateNode(nodeId, address)                
            $log.debug("Adding detached node: #{slotIndex} .. #{node} .. nodeId: #{nodeId}")
            @coreManager.registerDetachedNode(slotIndex, node)
        end
private 
        def connectToNode(node)
            # The connection attempt is done in a separate thread so that we do not block receiving
            # Better would be to make connection non-blocking (especially the auth-negotiation which actually uses message interconnect)
            ExceptionAwareThread.new() {
                nodeIpAddress = node.ipAddress
                $log.debug("Trying to connect to #{nodeIpAddress}")
                session = @trustManagement.authenticate(node.id)
                
                if session then
                    # TODO: Devel proof
                    succeeded = @filesystemConnector.connect(nodeIpAddress, session.authenticationProof)
                    $log.info("Connection attempt to #{nodeIpAddress} #{succeeded ? 'succeeded' : 'failed'}.")
                    if succeeded
                        slotIndex = @filesystemConnector.findDetachedManagerSlot(nodeIpAddress)
                        @detachedManagers[slotIndex] = DetachedNodeManager.new(node, slotIndex)
                    end
                end
            }
        end

end