# Class to be used for persistent ID sequence generations

class PersistentIdSequence    
    def initialize(idFile)
        # File where is the sequence state persisted
        @idFile = idFile
        @lock = Mutex.new
        
        load()
    end
    
    def nextId
        @lock.synchronize {
          number = @sequenceNumber
          @sequenceNumber = @sequenceNumber + 1
        
          save()
          return number
        }                
    end    
private
    def load()
        fileName = "#{@idFile}"

        if !File.exists?(fileName)
            @sequenceNumber = 1
	    save()
            return
        end
        
        File.foreach(fileName) { |line|
            @sequenceNumber = Integer(line)
        }                
    end
    
    def save()
        output = File.new("#{@idFile}", "w")
        output.puts @sequenceNumber
        output.close                
    end
end