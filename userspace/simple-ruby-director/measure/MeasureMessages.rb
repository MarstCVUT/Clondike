
# Message used for distribution of execution plans
class ExecutionPlanMessage
   attr_reader :executionPlan
   attr_reader :nodesToBlock
   attr_reader :initiatingNodeId

   def initialize(initiatingNodeId, executionPlan, nodesToBlock)
       @initiatingNodeId = initiatingNodeId
       @executionPlan = executionPlan
       @nodesToBlock = nodesToBlock
   end      
end

# Message with node summary of results from exeuction plan
class ExecutionResultMessage
   attr_reader :result
   attr_reader :nodeId
   attr_reader :taskIndex

   def initialize(nodeId, taskIndex, result)
       @nodeId = nodeId
       @taskIndex = taskIndex
       @result = result
   end      
end

# Just a notification message that current measurement has finished. It is used to unblock suspendend nodes after measurements is over.
class MeasurementFinished  
end