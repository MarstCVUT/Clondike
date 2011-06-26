class ExecutionHistory    
  attr_reader :totalDuration
  attr_reader :count
  
  def initialize(totalDuration, count)
    # Total duration of all observations
    @totalDuration = totalDuration
    # Count of observations
    @count = count
  end
  
  def addObservation(duration)
    @totalDuration = @totalDuration + duration
    @count = count + 1
  end
  
  def getPrediction()
    return @totalDuration / @count
  end
end

# Experimental class for estimating execution time based on classification
class ExecutionTimePredictor
  def initialize(confDir, learningMode)    
    # Whether the predictor is learning from execution results or just predicting
    # TODO: It would be better to have this dynamic flag, or possibly try to detect concurrency levels and see.
    @learningMode = learningMode
    loadPredictionData(confDir)
    
    if @learningMode then
      ExceptionAwareThread.new() {
	  saveThread(confDir);
      };
    end
  end
    
  # Returns esitmated execution time in seconds based on task data (task is a TaskInfo struture from task repository)
  # Returns nil in case prediction cannot be made
  def estimate(task)
     # TODO: Now we limit ourselves to comilation classifications as a proof-of-concept solution
     compileClassification = task.getClassificationOfType(CompileNameClassification)
     return nil if !compileClassification
     
     history = @predictions[compileClassification]
     return nil if !history
     
     return history.getPrediction()
  end
  
  def newTask(task)
    # Empty callback
  end
  
  def taskExit(task, exitCode)
     #outputPredictionMatch(task)
      
     return if !@learningMode
     compileClassification = task.getClassificationOfType(CompileNameClassification)
     return if !compileClassification
     
     history = @predictions[compileClassification]
     if ( !history ) then
       history = ExecutionHistory.new(task.duration, 1)
       @predictions[compileClassification] = history
     else
       history.addObservation(task.duration)
     end       
  end
  
private
  def outputPredictionMatch(task)
     compileClassification = task.getClassificationOfType(CompileNameClassification)
     return if !compileClassification

     history = @predictions[compileClassification]
     return if !history
     
     $log.debug("Task #{task.name} with classified compile file name #{compileClassification.compileFileName} was predicted #{history.getPrediction} vs real #{task.duration}")
  end

  def loadPredictionData(confDir)
    # Map keyed by classification and valued with ExecutionHistory
    @predictions = {}
    
    fileName = "#{confDir}/predictions.dat"
    fileName = "#{confDir}/predictions.dat.tmp" if !File.exists?(fileName)
    
    return if !File.exists?(fileName)
    
    IO.foreach(fileName) do |line| 
           name, duration, count = line.split(",")
	   @predictions[CompileNameClassification.new(name)] = ExecutionHistory.new(duration.to_f, count.to_i)
    end

  end
  
  def saveThread(confDir)
     fileName = "#{confDir}/predictions.dat"
     tmpFileName = "#{confDir}/predictions.dat.tmp"
    
     while ( true ) do
      sleep(120)
      File.open(tmpFileName, "w") { |file|            
	@predictions.each { |classification, history|
	    file.puts("#{classification.compileFileName}, #{history.totalDuration}, #{history.count}")
	}
      }
      
      File.delete(fileName) if File.exists?(fileName)
      File.rename(tmpFileName, fileName)
     end
  end
end