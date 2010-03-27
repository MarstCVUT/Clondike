require 'find'
require 'fileutils'

# Certificate record annotated with metadata
class CertificateRecord
    # The certificate itself
    attr_reader :certificate
    # Set of certificate records, that validates this certificate (if required)
    # Note, it is sufficient if only one of validating certificates if fully valid!
    attr_reader :validatedBy
    # Set of certificate records, validated by this records
    attr_reader :validates
    # Reference to a revocation certificate record, if any
    attr_accessor :revokedBy
    # True, if the record was valid last time checked, false if not, nil if not checked so far
    attr_reader :lastValidState        
    
    def initialize(certificate, certificateStore)
        @certificate = certificate
        @validates = Set.new()
        @validatedBy = Set.new()
        # Set of change listeners
        @store = certificateStore
    end
    
    def eql?(other)
        @certificate.issuer() == other.certificate.issuer && @certificate.id() == other.certificate.id
    end
    
    def hash()
        @certificate.issuer().hash + @certificate.id().hash
    end
    
    def reevalValidity()        
        isValidNow = isValid(@store.identity.publicKey)
        #puts "LAST VALID: #{@lastValidState} new valid #{isValidNow}"
        if ( isValidNow and (@lastValidState == nil or @lastValidState == false))
            @store.changeListener.becomeValid(certificate)
            reevalDependendRecords()
        elsif (!isValidNow and @lastValidState == true)
            @store.changeListener.becomeInvalid(certificate)
            reevalDependendRecords()
        end
        
        @lastValidState = isValidNow
    end        
    
    def becomeValidatorOf(record)
        raise "Cannot be self-validating" if ( record == self )
        
        @validates.add(record)
        record.validatedBy.add(self)
         
        record.reevalValidity()
        @store.distributionStrategy.onValidates(self, record)
        #@store.changeListener.becomeValid(record.certificate) if !wasValidBefore and record.isValid(@store.identity.publicKey)
    end
    
    def stopToBeValidatorOf(record)
        #wasValidBefore = record.isValid(@store.identity.publicKey)
        @validates.remove(record)
        record.validatedBy.remove(self)
         
        record.reevalValidity()        
        #@store.changeListener.becomeInvalid(record.certificate) if wasValidBefore and !record.isValid(@store.identity.publicKey)
    end
    
    def revokedBy=(value)
        #wasValidBefore = isValid(@store.identity.publicKey)
        @revokedBy = value
        reevalValidity()
        @store.distributionStrategy.onRevoces(value, self)
        # We use isValid to check if the revocation was valid
        #@store.changeListener.becomeInvalid(certificate) if wasValidBefore and !isValid(@store.identity.publicKey)
    end
    
    def isValid(localPublicKey)        
        checkCertificate = certificate
        #puts "Checking certificate certificate #{certificate.class}##{certificate.id}"
        return false if revokedBy != nil and isRevocationValid(localPublicKey, certificate, revokedBy.certificate)
        #puts "INVALID SIGNATURE for certificate #{certificate.class}##{certificate.id}"  if !checkCertificate.verifySignature()        
        return false if !checkCertificate.verifySignature()        
        return true if localPublicKey == checkCertificate.issuer
        return true if certificate.kind_of?(GroupMembershipCertificate) # No need for validation, anybody can make his own groups
        hasValidValidation = false
        validatedBy.each { |validatingCertificate|
            if ( validatingCertificate.isValid(localPublicKey)) then
                #puts "VALIDATOR FOUND: #{validatingCertificate.certificate.issuer}"
                hasValidValidation = true
                return true
            end
        }
        hasValidValidation
    end 

protected
    def isRevocationValid(localPublicKey, revocedCertificate, revocationCertificate)
        # Cannot revoke revocation certificates
        return false if revocedCertificate.kind_of?(RevocationCertificate)
        # Issuer of certificate can revoke it
        return true if ( revocationCertificate.issuer() == revocedCertificate.issuer())
        # Local user can revoke anything
        return true if revocationCertificate.issuer == localPublicKey
                            
        return false
    end

    def reevalDependendRecords
        @validates.each { |record|
            record.reevalValidity()
            record.reevalDependendRecords()
        }
    end
end

# All certificates issued by single node
class NodeCertificates
    # Public key of the issuing node
    attr_reader :publicKey
    # Hash of certificates  (id, certificateRecord)
    attr_reader :certificates
    # Set of certificate IDs revoked in scope of this node
    #attr_reader :revokedIds
    
    def initialize(publicKey)
        @publicKey = publicKey
        @certificates = {}
        #@revokedIds = Set.new()
    end
    
    def addCertificate(certificateRecord)
        # Already contained?
        return if ( @certificates[certificateRecord.certificate.id] != nil )        
        @certificates[certificateRecord.certificate.id] = certificateRecord
    end
    
    def addRevocation(revokationRecord)
        revocationId = revokationRecord.certificate.revokedId
        if ( @certificates[revocationId] )
            @certificates[revocationId].revokedBy = revokationRecord
        end
        #@revokedIds.add(revocationId)
    end
end

# Class storing all certificates known to this node
class CertificateStore
    # Local identity
    attr_reader :identity
    # Path to directory in FS, where are the certificates being stored
    attr_reader :directory
    # Strategy used for certificates distribution
    attr_reader :distributionStrategy
    # Hash of per node issued certificates (publicKey of the issuer, NodeCertificates structure)
    attr_reader :nodeCertificates
    # Change state listener (listening on even when a certificate becomes/ceases to be valid)
    attr_reader :changeListener
    
    def initialize(identity, distributionStrategy, directory)
        @identity = identity
        @distributionStrategy = distributionStrategy
        @directory = directory        
        @nodeCertificates = {}        
        @changeListener = GroupingCertificateListener.new
    end
    
    # Loads certificates from a directory specified 
    # (it is assumed the directory structure is managed by this store and so all certificates are stored where they should)
    def load()
        loadCertificateRecordsNoResolve()

        # While loading, we can check validations&revocations in one direction only, because we do it for all records
        eachCertificateRecord { |record|
            resolveValidates(record)
            resolveRevoces(record)
        }
    end
        
    def insertNewCertificate(certificate, persist)
        certificateRecord = addCertificateNoResolve(certificate)
        resolveRevoces(certificateRecord)
        resolveRevocedBy(certificateRecord)        
        resolveValidates(certificateRecord)
        resolveValidatedBy(certificateRecord)
        
        if ( persist )
            saveCertificateRecord(certificateRecord)
        end
        
        if ( @identity.publicKey == certificate.issuer )
            @distributionStrategy.onIssue(certificateRecord)
        else
            # Note: dispatching of receives in the store requires that ALL incoming
            # certificates are put into the store. It may not be desirable, but in
            # case it is changes the "onReceive" must be handled elsewhere
            @distributionStrategy.onReceive(certificateRecord)
        end
        
        return certificateRecord
    end
    
    def ackCertificate(issuer, id, acknowledger)
        certificateRecord = locateCertificate(issuer, id)
        return if !certificateRecord        
    end       
        
    # Remove in memory kept certificate, that is no longer required
    def removeCertificate(certificate)
        # TODO: Do we really need that? Even at a first prototype?
    end
    
    def allValidCertificates
         result = Set.new
         @nodeCertificates.each_value { |nodeCertificateStore|
             nodeCertificateStore.certificates.each_value { |certificateRecord|
                 result.add(certificateRecord.certificate) if ( certificateRecord.isValid(@identity.publicKey))
             }
         }   
         #puts "RES: #{result.size}"
         return result
    end
private         
    def loadCertificateRecordsNoResolve()
      # TODO: It would be likely better to process certificates in order with their sequence num (this would avoid possible conflicts)
      nonRevocations = Set.new
      Find.find(@directory) do |path|
          if File.basename(path).match(/\.cert/)
              certificate = loadCertificate(path)
              # First we process only revocations to avoid inserting some conflicting certificates
              if ( certificate.kind_of?(RevocationCertificate))
                  record = insertNewCertificate(certificate, false)
              else
                  nonRevocations.add(certificate)
              end
          end
      end
      
      # Later non revocations are processed  
      nonRevocations.each { |certificate|
          record = insertNewCertificate(certificate, false)
      }
    end

    def saveCertificateRecord(certificateRecord)
        certificate = certificateRecord.certificate
        saveDir = "#{directory}/#{OpenSSL::Digest::SHA1.new(certificate.issuer.undecorated_to_s)}"
        FileUtils::mkdir_p(saveDir) if ( !File.exists?(saveDir))
        certificate.save("#{saveDir}/#{certificate.id}.cert")
    end
    
    # Performs provided block op for every record in the store
    def eachCertificateRecord()
        @nodeCertificates.each_value { |nodeCertificate|
            nodeCertificate.certificates.each_value { |record|
                yield record
            }
        }
    end
    
    def locateCertificate(issuer, id)
        certificates = getOrCreateNodeCertificates(issuer)
        return certificates[id]
    end

    # Adds certificate record, but does not resolve any dependencies like validations, pending users etc..
    def addCertificateNoResolve(certificate)
        certificateRecord = CertificateRecord.new(certificate, self)
        certificateRecord.reevalValidity()
        #@changeListener.becomeValid(certificate) if ( certificateRecord.isValid(@identity.publicKey))
        certificates = getOrCreateNodeCertificates(certificate.issuer)
        certificates.addCertificate(certificateRecord)
        return certificateRecord
    end
    
    # Finds and marks all records that are validated by this certificate
    def resolveValidates(certificateRecord)
        certificate = certificateRecord.certificate
        # Only authorization certificates can delegate
        return if !certificate.kind_of?(AuthorizationCertificate) || !certificate.canDelegate
        authorizee = certificate.authorizee
        authorizeeCertificates = @nodeCertificates[authorizee]
        # Do we know any certificates from authorizee
        return if !authorizeeCertificates
        
        # Finally check all certificates from authorizee and see if they are validated
        authorizeeCertificates.certificates.each_value { |record|
            next if record == certificateRecord # Cannot validate self
            checkCertificate = record.certificate
            next if !checkCertificate.kind_of?(AuthorizationCertificate)
            # Note: We do not allow partial right delegations here!
            next if !areAllPermissionImplied(certificate.permissions, checkCertificate.permissions)
            # Check proper chaining
            next if (certificate.authorizee != checkCertificate.issuer)
            # Ok, this is a valid delegation
            certificateRecord.becomeValidatorOf(record)
        }
    end
    
    # Finds all records validating provided certificate records
    def resolveValidatedBy(certificateRecord)        
        certificate = certificateRecord.certificate
        return if !certificate.kind_of?(AuthorizationCertificate)
        
        # We have to go through all known certificates to see if some of them authorizes this newly added cert..
        eachCertificateRecord { |record|           
            next if record == certificateRecord # Cannot validate self
            checkCertificate = record.certificate
            next if !checkCertificate.kind_of?(AuthorizationCertificate) || !checkCertificate.canDelegate
            # Note: We do not allow partial right delegations here!
            next if !areAllPermissionImplied(checkCertificate.permissions, certificate.permissions)
            # Check proper chaining
            next if (checkCertificate.authorizee != certificate.issuer)
            # Ok, this is a valid delegation
            record.becomeValidatorOf(certificateRecord)                
        }
    end 

    # Resolves revocations made by this certificate
    def resolveRevoces(certificateRecord)    
        revocationCertificate = certificateRecord.certificate
        return if !revocationCertificate.kind_of?(RevocationCertificate)
        certificates = getOrCreateNodeCertificates(revocationCertificate.scope)
        certificates.addRevocation(certificateRecord)
    end
    
    # Finds a revocation certificate for this record, if it exists
    def resolveRevocedBy(certificateRecord)
        # We have to go through all known certificates
        eachCertificateRecord { |record|
            next if ( !record.certificate.kind_of?(RevocationCertificate) )
            next if record.certificate.scope != certificateRecord.certificate.issuer
            next if record.certificate.revokedId != certificateRecord.certificate.id
            certificateRecord.revokedBy = record
            return
        }
    end
    
    def getOrCreateNodeCertificates(publicKey)
        nodeCertificates = @nodeCertificates[publicKey]
        if ( !nodeCertificates ) then
            nodeCertificates = NodeCertificates.new(publicKey)
            @nodeCertificates[publicKey] = nodeCertificates
        end
        return nodeCertificates
    end
    
    # On certification creation, resolves all public keys that should recieve this certificate
    def resolveInterestedEntities(certificateRecord)
       interestedPublicKeys = Set.new
       case certificateRecord
          when GroupMembershipCertificate
              interestedPublicKeys.add(certificateRecord.certificate.member)
          when AuthorizationCertificate
              interestedPublicKeys.add(certificateRecord.certificate.authorizee)
              interestedPublicKeys.add(certificateRecord.certificate.target) if ( BACK_PROPAGATION )
          when RevocationCertificate
              interestedPublicKeys.add(certificateRecord.certificate.scope)
              
              #certificateRecord.validatedBy.each { |validatingRecord|
              #    interesterestedPublicKeys.merge()
              #}              
       end 
       
       return interestedPublicKeys
    end    
end

class GroupingCertificateListener
    def initialize()
        @listeners = Set.new
    end
    
    def addListener(listener)
        @listeners.add(listener)
    end
    
    def becomeValid(certificate)
        puts "Become valid: #{certificate}"
        @listeners.each { |listener|
            listener.becomeValid(certificate)
        }
    end
    
    def becomeInvalid(certificate)        
        puts "Become invalid: #{certificate}"
        @listeners.each { |listener|
            listener.becomeInvalid(certificate)
        }
    end        
end
