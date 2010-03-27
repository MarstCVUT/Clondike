#Class getting cpu usage since last time
class CpuUsageParser
    def initialize        
        @usageInfo = nil
        @oldUsageInfo = parseCurrentInfo()
    end

    def updateInfo
        @oldUsageInfo = @usageInfo
        @usageInfo = parseCurrentInfo()
    end
    
    def getUserUsage
        getUsage(0) + getUsage(1)
    end
    
    def getSystemUsage
        getUsage(2)
    end 

    def getIdle
        getUsage(3)
    end
private 
    def getUsage(index)
        return 0 if !@oldUsageInfo
        differences = each2(@usageInfo, @oldUsageInfo) { |a, b| a - b }
        totalSum = differences.inject(0) { |sum, element| sum += element }
        totalSum = 1 if totalSum < 1
        #puts "#{differences} / #{totalSum}"
        ((100*differences[index])/totalSum)
    end

    def each2(array1, array2)
        result = []
        for i in 0..array1.size-1
            result[i] = yield(array1[i], array2[i])
        end
        result
    end
    
    def parseCurrentInfo()
        cpuStat = []        
        IO.foreach("/proc/stat") { |line|
            if line =~ /^cpu/
                cpuStat << line.split(" ")[1..8]
            end            
        }                
        totalStat = [0,0,0,0,0,0,0,0]
        cpuStat.each { |stat|
            totalStat = each2(totalStat, stat) { |total, tmp| 
                total + tmp.to_i
            }            
        }
        
        #totalStat.each_with_index do |stat, index|
        #  puts "#{index} -> #{stat}!"
        #end
        
        totalStat
    end
end