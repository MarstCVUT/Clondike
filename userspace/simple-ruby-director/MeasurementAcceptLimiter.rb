# Limiter used in measurements, where we need to block some remote nodes so that only a certain volume of nodes participates
class MeasurementAcceptLimiter        
    # Flag used to block accepting of any remote tasks.. use in measurements
    attr_accessor :blocking
  
    def initialize()
      @blocking = false
    end
    
    def maximumAcceptCount
        return 100 if !@blocking
        return 0
    end    
end