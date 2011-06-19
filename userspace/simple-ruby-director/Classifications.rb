# Classifications can be used for predicting run-time behaviour of executables
class Classification
  # true, if the classification should be kept for forked childern
  attr_reader :surviveFork
  # true, if the classification should be kept after exec
  attr_reader :surviveExec
  
  def initialize(surviveFork, surviveExec)
    @surviveFork = surviveFork
    @surviveExec = surviveExec
  end
end

# Classifies a task with name of "c" file being compiled.. applies to cc1 only
class CompileNameClassification<Classification
  attr_reader :compileFileName
  
  def initialize(compileFileName)
      super(false, true)
      @compileFileName = compileFileName
  end
  
  def to_s()
    "CompileFile: #{@compileFileName}"
  end
end

class CompileNameClassificator
  def initialize
    @execPattern = Regexp.new("cc1$")
    @filenamePattern = Regexp.new("^[^\\/\\.]*\\.c$")
  end
  
  def classify(task)
    return false if !(task.name =~ @execPattern)
    fileName = nil
    task.args.each { |arg|
      if ( arg =~ @filenamePattern )
	      fileName = arg
              break                                      
      end
    }
    
    if ( fileName )
      task.addClassification(CompileNameClassification.new(fileName))
    end
  end
end

# Marker of task that are expected to run long time and can be migrated
class MigrateableLongTermTaskClassification<Classification
  def initialize()
    super(true, false)
  end
end

# Simple configurable classificator that assigns provided classification to all tasks with name matching execName pattern
class ExecNameConfigurableClassificator
  def initialize(execName, classification)
    @classification = classification
    @execPattern = Regexp.new(execName)
  end
  
  def classify(task)
    return false if !(task.name =~ @execPattern)
    
    task.addClassification(@classification)
  end
end