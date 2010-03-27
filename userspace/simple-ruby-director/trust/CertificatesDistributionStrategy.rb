# Dummy strategy, broadcasting all certificates to everybody
class BroadcastCertificateDistributionStrategy
    def initialize(interconnect)
        @interconnect = interconnect
    end
    
    def onIssue(certificateRecord)        
        broadcastCertificate(certificateRecord.certificate)
    end
    
    def onReceive(certificateRecord)        
    end    
    
    def onRevoces(revocingCertificateRecord, revocedCertificateRecord)
    end    
    
    def onValidates(validatingCertificateRecord, validatedCertificateRecord)
    end        
    
private
    def broadcastCertificate(certificate)
        message = CertificateMessage.new(certificate)
        # Broadcast to everybody
        $log.debug("Broadcasting message #{@interconnect != nil}")
        @interconnect.dispatch(nil, message) if @interconnect != nil        
    end
end

# This is the strategy to be used, that distributes certificates to the least
# subset of nodes that required them
class OptimizedCertificateDistributionStrategy
    def initialize(localPublicKey, interconnect)
        @localPublicKey = localPublicKey
        @interconnect = interconnect
    end
    
    def onIssue(certificateRecord)   
       case certificateRecord.certificate
          when GroupMembershipCertificate 
		groupMembershipIssued(certificateRecord)
          when GroupInterestCertificate 
		groupInterestIssued(certificateRecord)
          when AuthorizationCertificate 
		authorizationIssued(certificateRecord)          
          when RevocationCertificate 
		revocationIssued(certificateRecord)              
       end         
    end
    
    def onReceive(certificateRecord)        
        case certificateRecord.certificate
            when AuthorizationCertificate 
		authorizationReceived(certificateRecord)  
            when RevocationCertificate 
		revocationReceived(certificateRecord)              
        end
    end    
    
    def onRevoces(revocingCertificateRecord, revocedCertificateRecord)
    end    
    
    def onValidates(validatingCertificateRecord, validatedCertificateRecord)
    end        
    
private
end

# Helper class for optimized strategy, that resolves targets for a certificate
def OptimizedStrategyTargetResolver
    # Set of target public keys
    attr_reader :targetSet
    
    def initialize(certificateRecord)
        # Record, that is used to decide where shall be the message delivered
        # Note: Does not have to be the message itself!
        @certificateRecord = certificateRecord
        @targetSet = Set.new
    end
    
private
    def groupMembershipIssued(certificateRecord)
        sendMessageToEntity(certificateRecord.certificate.member)
        sendMessageToGroupListeners(@localPublicKey, certificateRecord.certificate.groupName)
    end
    
    def authorizationIssued(certificateRecord)
        certificate = certificateRecord.certificate
        sendMessageTo(certificate.authorizee, certificate.authorizeeGroupName)
        sendMessageTo(certificate.target, certificate.targetGroupName)
    end
    
    def groupInterestIssued(certificateRecord)
        sendMessageToEntity(certificateRecord.certificate.master)
    end
    
    def revocationIssued(certificateRecord)
    
    end
    
    def authorizationReceived(certificateRecord)
        certificate = certificateRecord.certificate
        # In case either authorizee or target of the certificate was some local group,
        # we are responsible for forwarding the certificate to all members of group
        sendMessageIfLocalGroup(certificate.authorizee, certificate.authorizeeGroupName)
        sendMessageIfLocalGroup(certificate.target, certificate.targetGroupName)
    end
    
    def revocationReceived(certificateRecord)
        # TODO
    end

    # Sends message to either a single node or a group
    def sendMessageTo(publicKey, groupName)
        groupName == nil ? sendMessageToEntity(publicKey) : sendMessageToGroup(publicKey, groupName)
    end
    
    # Sends message to a single group
    def sendMessageToGroup(publicKey, groupName)
        groupEntity = @dataStore.resolveGroupEntity(publicKey, groupName)
        isLocalGroup(groupEntity) ? sendMessageToLocalGroup(groupEntity) : sendMessageToRemoteGroup(groupEntity)
    end

    # In case params specify one of local groups, the message is send to all its members
    # Otherwise nothing is done
    def sendMessageIfLocalGroup(masterKey, groupName)
        return if !groupName
        return if !masterKey == @locakPublicKey
        groupEntity = @dataStore.resolveGroupEntity(masterKey, groupName)
        sendMessageToLocalGroup(groupEntity)
    end
    
    # Sends message to all members of local group
    def sendMessageToLocalGroup(groupEntity)
        groupEntity.eachEntity { |entity|
            raise "Assertion failed" if (!entity.name.kind_of?(PublicKey))
            sendMessageToEntity(entity.name)
        }
    end

    # Sends message to a remote group
    def sendMessageToRemoteGroup(groupEntity)
        # Simply send message to master of that group. so that he can forward it to
        # all members of the group
        sendMessageToEntity(groupEntity.scope)
    end
    
    # Sends message to all enities interested in group membership changes
    def sendMessageToGroupListeners(publicKey, groupName)
        groupEntity = @dataStore.resolveGroupEntity(publicKey, groupName)
        groupEntity.eachInterestedKey { |key|
            sendMessageToEntity(key)
        }
    end    
    
    # Sends message to a single entity
    def sendMessageToEntity(publicKey)
        message = CertificateMessage.new(certificate)
        #$log.debug("Sending message #{certificate.class} to #{publicKey}")
        #@interconnect.dispatch(nil, message) if @interconnect != nil                
        @targetSet.add(publicey)
    end
    
    def isLocalGroup(groupEntity)
        return groupEntity.kind_of?(LocalGroupEntity)
    end
end