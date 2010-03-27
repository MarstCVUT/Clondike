# Notification target resolvers are used to detect which entity should be notified
# about a new certificate or certificate state change

class EmptyTargetResolver
    def onIssue(certificateRecord)        
    end
    
    def onReceive(certificateRecord)        
    end    
    
    def onRevoces(revocingCertificateRecord, revocedCertificateRecord)
    end    
    
    def onValidates(validatingCertificateRecord, validatedCertificateRecord)
    end        
end

# Resolver for group membership certificates
class GroupTargetResolver
    def onIssue(certificateRecord)        
        # On group membership issue, notify a member of the group about 
        certificateRecord.addPendingAck(certificateRecord.certificate.member))
    end
    
    def onReceive(certificateRecord)        
    end    
    
    def onRevoces(revocingCertificateRecord, revocedCertificateRecord)
    end    
    
    def onValidates(validatingCertificateRecord, validatedCertificateRecord)
    end        
end
