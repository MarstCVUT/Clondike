require "openssl"
require "trust/Certificates.rb"
require "trust/CertificateStore.rb"
require 'trust/Permission.rb'
require 'PersistentIdSequence.rb'
require 'delegate.rb'
require "set"

# Class representing all identity related aspects of node
class Identity    
    # Private key of the node
    attr_reader :privateKey
    # Public key of the node
    attr_reader :publicKey
    # Certificate corresponding to public key
    attr_reader :certificate
    # Store with all certificates known by this identity
    attr_reader :certificateStore
    # Directory where to store all certificates
    attr_reader :directory
           
    # Loads an identity data from a specified path
    # Returns nil in case identity data cannot be loaded at that path
    def self.loadIfExists(directory, distributionStrategy)
        return nil if ( !identityExists(directory) )

        identity = Identity.new(directory, distributionStrategy, nil, nil, nil)
        identity.load()
        return identity
    end
    
    # Saves an identity data at a specified path
    def save()        
        saveKey(publicKey,"#{@directory}/public.pem")
        saveKey(privateKey,"#{@directory}/private.pem")
        certificate.save("#{@directory}/identity.cert")
        # TODO: Save remaining certs
    end
    
    def load()
        @publicKey = loadKey("#{directory}/public.pem")
        @privateKey = loadKey("#{directory}/private.pem")
        @certificate = loadCertificate("#{directory}/identity.cert")
        @certificateStore.load()
    end    
    
    # Creates and returns a new identity data
    def self.create(directory, distributionStrategy)
        newKey = RSAPublicKey.generate(1024)
        
        Identity.new(directory, distributionStrategy, newKey, newKey.public_key, NodeCertificate.new(0, newKey.public_key))
    end

    def issueAliasCertificate(nodeAlias, nodePublicKey)
        certificate = AliasCertificate.new(nextSequenceNumber(), @publicKey, nodeAlias, nodePublicKey)
        certificate.sign(privateKey)
        @certificateStore.insertNewCertificate(certificate, true)
        return certificate
    end
    
    def issueGroupMembershipCertificate(groupName, member)
        certificate = GroupMembershipCertificate.new(nextSequenceNumber(), @publicKey, groupName, member)
        certificate.sign(privateKey)
        @certificateStore.insertNewCertificate(certificate, true)
        return certificate
    end
    
    # Permissions can be either a set of permissions or a single permission
    def issueAuthorizationCertificate(authorizee, authorizedGroupName, target, targetGroupName, permissions, canDelegate)
        # Wrap permissions into a collection, if required
        if ( !permissions.kind_of?(Set))
            permissionsSet = Set.new
            permissionsSet.add(permissions)
            permissions = permissionsSet
        end
        
        certificate = AuthorizationCertificate.new(nextSequenceNumber(), @publicKey, authorizee, authorizedGroupName, target, targetGroupName, permissions, canDelegate)
        certificate.sign(privateKey)
        @certificateStore.insertNewCertificate(certificate, true)
        return certificate        
    end

    def revokeCertificate(certificateToRevoke)    
        #puts "REVOKING #{certificateToRevoke.issuer} .... #{@publicKey}"
        certificate = RevocationCertificate.new(nextSequenceNumber(), @publicKey, certificateToRevoke.issuer, certificateToRevoke.id)
        certificate.sign(privateKey)
        @certificateStore.insertNewCertificate(certificate, true)
        return certificate                
    end
    
    # Should be called whever we recieve a new certificate from some other node
    def receiveCertificate(certificate)
        # TODO: Should we always persist? There are some certificates that shall not be persisted!
        @certificateStore.insertNewCertificate(certificate, true)
    end
    
    # Signs arbitrary (string) data and returns a signature
    def sign(dataToSign)
        @privateKey.sign(OpenSSL::Digest::SHA1.new, dataToSign)
    end

    def decrypt(encryptedData)
	@privateKey.private_decrypt(encryptedData)
    end
protected    
    def initialize(directory, distributionStrategy, privateKey, publicKey, certificate)
        @directory = directory
        @privateKey = privateKey
        @publicKey = publicKey
        @certificate = certificate
        
        @certificateStore = CertificateStore.new(self, distributionStrategy, @directory)
        
        # Sequence used for certificate ID generation
        @sequenceId = PersistentIdSequence.new("#{directory}/seq")
    end
        
    # Returns true, if identity files exists at specified directory
    def self.identityExists(directory)
        # TODO: We now check presence of sequence only. It'd be better to perform more checks..
        return File.exists?("#{directory}/seq")
    end
    
    def nextSequenceNumber
        @sequenceId.nextId
    end
        
    def saveKey(key,file)
        output = File.new(file, "w")
        output.puts key.full_to_s
        output.close
    end
    
    def loadKey(file)
        RSAPublicKey.new(File.read(file))
    end
end

def splitBase64(string)
    r = []
    len = 64
    start = 0
    while(start <= string.length) do                                              
      r << string[start...start+len] 
      start += len 
    end            
    return r.join("\n")
end

def unsplitBase64(string)
    res = string.gsub(/\n/,"")
end

class RSAPublicKey < SimpleDelegator
	def initialize(decoratedKeyText)
		super(decoratedKeyText != nil ? OpenSSL::PKey::RSA.new(decoratedKeyText) : nil)
	end

    def undecorated_to_s
        #puts "BEFORE UNDECOR: #{to_s}"
        res = __getobj__.to_s
        res = res.sub("-----BEGIN RSA PUBLIC KEY-----","")
        res = res.sub("-----END RSA PUBLIC KEY-----","")
        unsplitBase64(res)
        #res = res.gsub("\r","")
    end        
    
    def self.undecoratedLoad(undecoratedPemKey)        
        decoratedText = decoratedKey(undecoratedPemKey)
        #puts "DEC TEXT #{decoratedText}"
        RSAPublicKey.new(decoratedText)
    end
    
    def public_key
        RSAPublicKey.new(super.public_key.to_s)
    end
    
    def marshal_dump()
        return undecorated_to_s
    end
    
    def marshal_load(var)
        #puts "UNMARSHALING: #{var} #{var.class}"
	rsaKey = OpenSSL::PKey::RSA.new(RSAPublicKey.decoratedKey(var))
        __setobj__(rsaKey)
    end
    
    def hash()
        return undecorated_to_s.hash
    end

    # Short-cut method with hardcoded digest algorithm
    def verifySignature(signature, data)
	verify( OpenSSL::Digest::SHA1.new, signature, data)
    end
    
    def self.generate(number)
        rsaKey = RSAPublicKey.new(nil)
        rsaKey.__setobj__(OpenSSL::PKey::RSA.generate(number))
        return rsaKey
    end
    
    def ==(other)
        #puts "AAA: #{self.class} other #{other.class} ... ??? #{other.kind_of?(OpenSSL::PKey::RSA)}"
        #puts "COOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO"
        return false if !other.kind_of?(RSAPublicKey)
        #puts "MPPAAA"
        #puts "#{self}"
        #puts "#{other}"
        #sputs "EQ? #{undecorated_to_s == other.undecorated_to_s}"
        
        return undecorated_to_s == other.undecorated_to_s        
    end   

    def toShortHashString()
      return OpenSSL::Digest::SHA1.new(undecorated_to_s)
    end
    
    def full_to_s
      return __getobj__.to_s
    end
    
    def to_s
      return toShortHashString().to_s
    end
    
    def eql?(other)
        self == other
    end
    
private
    def self.decoratedKey(undecoratedPemKey)
        "-----BEGIN RSA PUBLIC KEY-----\n#{splitBase64(undecoratedPemKey)}\n-----END RSA PUBLIC KEY-----\n"
    end
end