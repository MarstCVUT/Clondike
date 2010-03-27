# HeartBeatMessages send through a kernel link channels
class HeartBeatMessage
	# Id of the sender node
	attr_reader :nodeId

	def initialize(nodeId)
		@nodeId = nodeId
	end
end