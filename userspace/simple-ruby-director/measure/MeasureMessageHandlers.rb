
class ExecutionResultHandler
    def initialize(measurementDirector)
      @measurementDirector = measurementDirector
    end
    
    # Handles ExecutionResultMessage
    def handle(message)
        $log.debug("Received execution result message.")
        
	measurement = @measurementDirector.currentMeasurement
	measurement.addResult(message.nodeId, message.taskIndex, message.result)
    end  
end

class ExecutionResultListener
  def initialize(localNodeId, initiatingNodeId, interconnect)
    @localNodeId = localNodeId
    @initiatingNodeId = initiatingNodeId
    @interconnect = interconnect
  end
  
  def notifyNewResult(taskIndex, result)
     resultMessage = ExecutionResultMessage.new(@localNodeId, taskIndex, result)
     @interconnect.dispatch(@initiatingNodeId, resultMessage)
  end
end

class ExecutionPlanHandler
  def initialize(localNodeId, interconnect)
    @localNodeId = localNodeId
    @interconnect = interconnect
  end
  
  def handle(message)
      $log.debug("Received execution plan message.")
      
      message.executionPlan.execute(@localNodeId, ExecutionResultListener.new(@localNodeId, message.initiatingNodeId, @interconnect))
  end  
end