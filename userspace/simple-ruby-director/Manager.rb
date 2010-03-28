DETACHED_MANAGER_SLOT = 0
CORE_MANAGER_SLOT = 1

class ManagerSlot
	attr_reader :slotIndex
	attr_reader :slotType

	def initialize(slotType, slotIndex)
		@slotIndex = slotIndex
		@slotType = slotType
	end

	def to_s
		"#{slotType}/#{slotIndex}"
	end
end

class DetachedNodeManager
	# Associated core node information (i.e. the core node on the "other end")
        # Holds node objects
	attr_accessor :coreNode
	# Slot, in which resides connection to the associated core node
	attr_reader :coreNodeSlotIndex

	def initialize(coreNode, coreNodeSlotIndex)
		@coreNode = coreNode
                @coreNodeSlotIndex = coreNodeSlotIndex
	end
end

class CoreNodeManager
	# All associated detached nodes. They should be placed to their associated slots in the array
        # Holds Node objects
	attr_reader :detachedNodes

	def initialize
		@detachedNodes = []
	end
                
	def registerDetachedNode (slotIndex, node)
		raise(ArgumentError,"Node already registered at index #{slotIndex}") if @detachedNodes[slotIndex] != nil
		@detachedNodes[slotIndex] = node
	end

	def unregisterDetachedNode (slotIndex)
		raise(ArgumentError, "No node registered at index #{slotIndex}") if @detachedNodes[slotIndex] == nil
		@detachedNodes[slotIndex] = nil
	end
        
        def nodeConnected(address, slotIndex)
            
        end
end