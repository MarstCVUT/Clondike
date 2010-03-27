require 'MonitoringMessages'

# This class is responsible for monitoring node connectivity
# TODO: At the moment, there is no monitoring functionality, just periodic heartbeating that sends node ids
class ManagerMonitor	
	def initialize(interconnection, membershipManager, nodeRepository)
		@interconnection = interconnection
		@membershipManager = membershipManager
		@currentNodeId = nodeRepository.selfNode.id
		# In seconds
		@heartBeatPeriod = 5

	        @interconnection.addReceiveHandler(HeartBeatMessage, HeartBeatHandler.new(membershipManager, nodeRepository)) if ( interconnection )
	end

	# Starts background threads
	def start
		Thread.new() {
			begin
				while true do
					emitHeartBeats()
					sleep @heartBeatPeriod # This is not correct, we should actually sleep just the remaining portion of time..
				end
			rescue => err
				$log.error "Error in monitor thread: #{err.message} \n#{err.backtrace.join("\n")}"
			end
		}
	end
private
	def emitHeartBeats
		@membershipManager.coreManager.detachedNodes.each_with_index { |element, slotIndex|
			next if !element
			emitHeartBeat(CORE_MANAGER_SLOT, slotIndex, element);
		}

		@membershipManager.detachedManagers.each_with_index { |element, slotIndex|
			next if !element
			emitHeartBeat(DETACHED_MANAGER_SLOT, slotIndex, element.coreNode);
		}
	end

	def emitHeartBeat(slotType, slotIndex, node)		
		managerSlot = ManagerSlot.new(slotType, slotIndex)	
		if ( !node )
			$log.warn("Failed to resolve node for slot #{managerSlot}")
		end

		message = HeartBeatMessage.new(@currentNodeId)
		@interconnection.dispatchToSlot(managerSlot, message)
	end
end

class HeartBeatHandler
	def initialize(membershipManager, nodeRepository)
		@membershipManager = membershipManager
		@nodeRepository = nodeRepository
	end

	# TODO: Proper sync of this method?
	def handleFrom(heartBeatMessage, fromManagerSlot)
		nodeId = heartBeatMessage.nodeId
		if ( fromManagerSlot.slotType == CORE_MANAGER_SLOT )
			node = @membershipManager.coreManager.detachedNodes[fromManagerSlot.slotIndex]
			if node.id == nil
				$log.debug "Updated core node id #{nodeId} from a heartbeat message"
 				newNode, isNew = @nodeRepository.getOrCreateNode(nodeId, node.ipAddress)
				@membershipManager.coreManager.unregisterDetachedNode(fromManagerSlot.slotIndex)
				@membershipManager.coreManager.registerDetachedNode(fromManagerSlot.slotIndex,newNode)
			end
		else
			node = @membershipManager.detachedManagers[fromManagerSlot.slotIndex].coreNode
			if node.id == nil
				$log.debug "Updated detached node id #{nodeId} from a heartbeat message"
 				newNode, isNew = @nodeRepository.getOrCreateNode(nodeId, node.ipAddress)
				@membershipManager.detachedManagers[fromManagerSlot.slotIndex].coreNode = newNode
			end
		end		
	end
end
