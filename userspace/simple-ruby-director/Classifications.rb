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