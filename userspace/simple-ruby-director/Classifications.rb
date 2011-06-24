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
  
  def ==(other)
    other.class == self.class && @compileFileName == other.compileFileName
  end  

  def eql?(other)
    other.class == self.class && @compileFileName == other.compileFileName
  end  
  
  def hash()
    @compileFileName.hash
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

# Base class for all classifications with no parameters, provides ready to use implementation of equals and hash
class NonparametricClassification<Classification
  def initialize(surviveFork, surviveExec)
    super(surviveFork, surviveExec)
  end
  
  def ==(other)
    other.class == self.class
  end  

  def eql?(other)
    other.class == self.class
  end  
  
  def hash()
    self.class.hash
  end    
end

# Marker of task that are expected to run long time and can be migrated
class MigrateableLongTermTaskClassification<NonparametricClassification
  def initialize()
    super(true, false)
  end    
  
  def to_s
    return "MigrateableLongTermTask"
  end  
end
         
         
# Marker for tasks that serve as a "Master" of master worker calculation
# Such a tasks may not be a good candidates for migration so we mark them separately         
class MasterTaskClassification<NonparametricClassification
  def initialize()
    super(false, false)
  end    
  
  def to_s
    return "MasterTask"
  end
end         

class ChildNoClassification<Classification
   def initialize(childNo)
    super(false, false)
    
    @childNo = childNo
  end    

  def ==(other)
    other.class == self.class && @childNo == other.childNo
  end  

  def eql?(other)
    other.class == self.class && @childNo == other.childNo
  end  
  
  def hash()
    @childNo.hash
  end      
end

# Simple configurable classificator that assigns provided classification to all tasks with name matching execName pattern
class ExecNameConfigurableClassificator
  def initialize(execName, classifications, classifyChildrenByNo = false)
    @classifications = classifications
    @execPattern = Regexp.new(execName)
    # If true, children tasks get ChildNoClassification
    @classifyChildrenByNo = classifyChildrenByNo
  end
  
  def classify(task)
    return false if !(task.name =~ @execPattern)
    @classifications.each { |classification|
	task.addClassification(classification)
        parent = task.parent
        if ( parent ) then
	    task.addClassification(ChildNoClassification.new(parent.forkCount)) if parent.hasClassificiation(classification )
        end
    }                               
  end
end