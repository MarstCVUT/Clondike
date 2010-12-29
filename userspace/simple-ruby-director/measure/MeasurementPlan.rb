
# Interface representing a command to be executed as a part of measurement
class MeasurementCommand  
end

class ExecuteCommand < MeasurementCommand
  def initialize(execCommand, execPath)
    @command = execCommand
    @path = execPath
  end
  
  def runCommand()    
   pid = fork { exec @command }
   Process.waitpid(pid, 0) 
  end
  
  def to_s
    @command
  end
end

class DisconnectCommand < MeasurementCommand
  def initialize(disconnectSeconds)
    @disconnectSeconds = disconnectSeconds;
  end
  
  def runCommand()
    #TODO
  end
end

class RebootCommand < MeasurementCommand
  def runCommand()
    #TODO
  end
end

# Single task to be performed as a part of measurement
class MeasurementTask
  attr_reader :command
  attr_reader :startTimeOffset
  attr_reader :result

  def initialize(command, startTimeOffset)
    raise "Command must not be null" if !command
    
    @command = command
    # Offset in secods when to start (with respect to measrument plan start time)
    @startTimeOffset = startTimeOffset
  end
  
  # Associate execution result with the task
  def setResult(result)
    @result = result
  end 
  
  def execute(index, resultListener)
    startTime = Time.now.to_f
    Thread.new() {
      begin
	$log.debug "Starting execution of command: #{command}"
	@command.runCommand()
	$log.debug "Finished execution of command: #{command}"
      
	endTime = Time.now.to_f
	result = MeasurementTaskResult.new(startTime, endTime)
	resultListener.notifyNewResult(index, result)
      rescue => err
	$log.error "Error in command execution thread: #{err.message} \n#{err.backtrace.join("\n")}"
      end  
    }
  end
end

# Measurement plan for a single node
class NodeMeasurementPlan    
  def initialize(nodeId)
    @nodeId = nodeId
    @tasks = []
    @executedTaskIndexes = Set.new
  end
  
  def addTask(task) 
    @tasks << task
  end    
  
  def addResult(taskIndex, result)
    @tasks[taskIndex].setResult(result)
  end
  
  def execute(planStartTime, resultListener)                
    Thread.new() {
      begin
	$log.debug "Looking for a best task to execute"
	nextTaskIndex = findNextTaskToExecute(planStartTime)
	while nextTaskIndex do
	  waitSeconds = planStartTime + @tasks[nextTaskIndex].startTimeOffset - Time.now.to_f
	  $log.debug "Best task to execute: #{@tasks[nextTaskIndex]} -> Wait time: #{waitSeconds} sec."
	  sleep waitSeconds if waitSeconds > 0
	  @tasks[nextTaskIndex].execute(nextTaskIndex, resultListener)
	  @executedTaskIndexes.add(nextTaskIndex)
	      
	  nextTaskIndex = findNextTaskToExecute(planStartTime)
	end    
      rescue => err
	$log.error "Error in plan execution thread: #{err.message} \n#{err.backtrace.join("\n")}"
      end  
    
      $log.debug "No more tasks to be executed on this machine"
    }
  end
  
  def saveToFile(file, planStartTime, nodeName)
    file.puts("Name: #{nodeName}")
    @tasks.each { |task|
        file.puts("  Command: #{task.command} Should start: #{Time.at(planStartTime + task.startTimeOffset)} Started: #{Time.at(task.result.startTime)} Finished: #{Time.at(task.result.endTime)}")
    }    
  end
private
  def findNextTaskToExecute(planStartTime)
    lowestStartTime = nil
    bestIndex = nil
    @tasks.each_with_index { |task, index|
        next if ( @executedTaskIndexes.include?(index) )
        startTime = planStartTime + task.startTimeOffset
                
        if ( lowestStartTime == nil or lowestStartTime > startTime )
	    lowestStartTime = startTime
	    bestIndex = index
        end                
    }
    return bestIndex
  end
end

class LocalResultListener
    def initialize(nodeId, measurement)
      @nodeId = nodeId
      @measurement = measurement
    end
                
    def notifyNewResult(taskIndex, result)
	@measurement.addResult(@nodeId, taskIndex, result)
    end
end
                
                
# Plan for a measurement containing all required information
class MeasurementPlan   
  def initialize(startTime)
    # Maps nodeId to NodeMeasurementPlan
    @nodePlanMapping = {}
    @startTime = startTime
    
    @requiredResultCount = 0
    @receivedResultCount = 0
  end
  
  def addNodeTaskAssignement(nodeId, task)
    plan = getOrCreateNodeMeasurementPlan(nodeId)
    plan.addTask(task)    
    
    @requiredResultCount = @requiredResultCount + 1
  end
  
  def addResult(nodeId, taskIndex, result)
    plan = @nodePlanMapping[nodeId]
    plan.addResult(taskIndex, result)
        
    # TODO: It would be safer to check that we actually got proper result, but for now OK like this
    @receivedResultCount = @receivedResultCount + 1       
    
    $log.debug("Got result. Now has #{@receivedResultCount} results out of #{@requiredResultCount} required")
  end
    
  def hasAllResults()
    return @receivedResultCount >= @requiredResultCount
  end
  
  # This method will process all tasks associated with the node specified by localNodeId
  # Execution is asynchronous, passing back results to listener
  def execute(localNodeId, resultListener)
    plan = @nodePlanMapping[localNodeId]
    if ( plan )
      plan.execute(@startTime, resultListener)
    else
      $log.debug("No task for current node")
    end
  end  
                
  def saveToFile(file, nodeId, nodeName)
    plan = @nodePlanMapping[nodeId]
    plan.saveToFile(file, @startTime, nodeName)
  end
private
  def getOrCreateNodeMeasurementPlan(nodeId)
     plan = @nodePlanMapping[nodeId]
     if ( !plan )
	plan = NodeMeasurementPlan.new(nodeId)
	@nodePlanMapping[nodeId] = plan
     end
     return plan
  end           
end

class Measurement
  attr_reader :measurementPlan
           
  def initialize(startTime, outputFileName)
    # Used to assign unique ids to tasks    
    @measurementPlan = MeasurementPlan.new(startTime)
    @outputFileName = outputFileName
    @nodeMapping = {}    
  end
  
  def buildNodeMappingForAllKnownNodes(nodeRepository)
    buildNodeMapping(nodeRepository, -1)
  end
  
  def buildNodeMapping(nodeRepository, requiredNodeCount)
    nodeCount = 0
    availableNodes = nodeRepository.knownNodesCount + 1 # +1 for "self"
    raise "Not enough nodes. Available #{availableNodes}, but required #{requiredNodeCount}" if availableNodes < requiredNodeCount
    
    name = "LocalNode"
    @nodeMapping[name] = nodeRepository.selfNode
    nodeCount = nodeCount + 1
    
    nodeRepository.eachNode { |node|
      break if requiredNodeCount > 0 and requiredNodeCount == nodeCount
                                        
      name = "RemoteNode#{nodeCount}"
      @nodeMapping[name] = node                             
      nodeCount = nodeCount + 1                            
    }        
    
    # Secondary check, in case known nodes count has decreased during the iteration
    raise "Not enough nodes. Found #{nodeCount}, but required #{requiredNodeCount}" if ( nodeCount < requiredNodeCount )
  end
  
  def addNodeTaskAssignement(nodeName, task)
    node = @nodeMapping[nodeName]
    raise "Node '#{nodeName}' not found in name mapping. You have to initialize node mapping via buildNodeMapping with sufficiently large requiredNodeCount" if !node
    
    @measurementPlan.addNodeTaskAssignement(node.id, task)
  end
  
  def addAllNodesTaskAssignement(task)
    @nodeMapping.each_value { |node|
	@measurementPlan.addNodeTaskAssignement(node.id, task)
    }
  end
  
  def addResult(nodeId, taskIndex, result)
    @measurementPlan.addResult(nodeId, taskIndex, result)
                    
    if ( hasAllResults() )
      saveResults()
      $log.info "Results saved and measurement finished"
    end
  end                      
  
  def hasAllResults
    @measurementPlan.hasAllResults
  end  
                
  def executeLocally(localNodeId)
    @measurementPlan.execute(localNodeId, LocalResultListener.new(localNodeId, self))
  end                
    
private                
  def saveResults()
    File.open(@outputFileName, "w") { |file|            
	file.puts("-------------------- Execution results ----------------------")
	@nodeMapping.each { |name, node|
            @measurementPlan.saveToFile(file, node.id, name)	    
	}
        file.puts("----------------------- End of results ----------------------")
    }                   
  end                
end

class MeasurementTaskResult
  attr_reader :startTime                
  attr_reader :endTime
                
  def initialize(startTime, endTime)
    @startTime = startTime
    @endTime = endTime
  end    
end