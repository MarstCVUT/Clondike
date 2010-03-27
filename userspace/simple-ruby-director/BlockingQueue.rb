require 'thread'

#This is a thread-safe queue, that supports blocking dequeue call
class BlockingQueue
    def initialize
       @queue = []
       @queueLock = Mutex.new()        
       @queueCondition = ConditionVariable.new()
    end
    
    #Enqueues a new element at the end of the queue
    def enqueue element
        @queueLock.synchronize { 
            @queue.unshift(element)
            @queueCondition.signal()
        }
    end
    
    #Blocking dequeing of element
    def dequeue
        @queueLock.synchronize { 
            while @queue.empty?
                @queueCondition.wait(@queueLock)
            end             
            @queue.pop            
        }
        
    end
end