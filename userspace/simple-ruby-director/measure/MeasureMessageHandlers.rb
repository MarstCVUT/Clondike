
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
  def initialize(localNodeId, interconnect, measurementDirector)
    @localNodeId = localNodeId
    @interconnect = interconnect
    @measurementDirector = measurementDirector
  end
  
  def handle(message)
      $log.debug("Received execution plan message.")
      if message.nodesToBlock.include?(@localNodeId)
	  $log.debug("Suspending node for the time of measurement.")
	  @measurementDirector.suspendNode
      else
	  message.executionPlan.execute(@localNodeId, ExecutionResultListener.new(@localNodeId, message.initiatingNodeId, @interconnect))
	  $log.debug("Measurement plan execution started in back-ground.")
      end      
  end  
end

class MeasurementFinishedHandler
    def initialize(measurementDirector)
      @measurementDirector = measurementDirector
    end
    
    def handle(message)
        $log.debug("Resuming node after calculation completition.")
        @measurementDirector.resumeNode
    end  
end
