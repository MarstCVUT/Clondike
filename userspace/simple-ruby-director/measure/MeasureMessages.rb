
# Message used for distribution of execution plans
class ExecutionPlanMessage
   attr_reader :executionPlan
   attr_reader :initiatingNodeId

   def initialize(initiatingNodeId, executionPlan)
       @initiatingNodeId = initiatingNodeId
       @executionPlan = executionPlan
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