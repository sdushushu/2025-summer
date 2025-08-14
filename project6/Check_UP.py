import hashlib
from sympy import randprime
import secrets
import random
from phe import paillier
from functools import lru_cache
import math

class CryptoBase:
    prime_number = None
    _prime_cache = {}
    
    @classmethod
    def _generate_prime_safely(cls, bit_length):
        if bit_length in cls._prime_cache:
            return cls._prime_cache[bit_length]
            
        min_val = 2 ** (bit_length - 1)
        max_val = 2 ** bit_length - 1
        prime_candidate = randprime(min_val, max_val)
        
        if not cls._verify_prime(prime_candidate):
            raise ValueError("生成的候选数不是素数")
            
        cls._prime_cache[bit_length] = prime_candidate
        return prime_candidate

    @staticmethod
    def _verify_prime(num, certainty=5):
        if num < 2: return False
        if num in (2, 3): return True
        if num % 2 == 0: return False
        
        exponent, remainder = num - 1, 0
        while exponent % 2 == 0:
            exponent //= 2
            remainder += 1
            
        for _ in range(certainty):
            base = secrets.randbelow(num - 3) + 2
            result = pow(base, exponent, num)
            if result in (1, num - 1):
                continue
            for __ in range(remainder - 1):
                result = pow(result, 2, num)
                if result == num - 1:
                    break
            else:
                return False
        return True

    @classmethod
    def initialize(cls, bit_length=1024):
        cls.prime_number = cls._generate_prime_safely(bit_length)
    
    @lru_cache(maxsize=1024)
    def modular_exponentiation(self, base, exponent):
        return pow(base, exponent, self.prime_number)
    
    def create_secret_key(self):
        if self.prime_number is None:
            raise ValueError("请先调用 initialize() 初始化素数")
        return secrets.randbelow(self.prime_number - 2) + 1
    
    def credential_to_int(self, credential):
        if self.prime_number is None:
            raise ValueError("请先调用 initialize() 初始化素数")
        hashed = hashlib.blake2b(credential.encode(), digest_size=32).digest()
        return int.from_bytes(hashed, 'big') % self.prime_number
    
    def process_credentials(self, credentials):
        if self.prime_number is None:
            raise ValueError("请先调用 initialize() 初始化素数")
        return [self.credential_to_int(cred) for cred in credentials]
    
    def create_crypto_keys(self):
        return paillier.generate_paillier_keypair()
    
    def encrypt_data(self, data, public_key):
        return public_key.encrypt(data)
    
    def decrypt_data(self, encrypted_data, private_key):
        return private_key.decrypt(encrypted_data)

class Initiator(CryptoBase):
    def __init__(self, credentials=None):
        self.user_credentials = credentials or {"securePass123"}
        self.secret_value = self.create_secret_key()
        self.hashed_credentials = self.process_credentials(self.user_credentials)
        self.precomputed_values = [self.modular_exponentiation(h, self.secret_value) 
                                  for h in self.hashed_credentials]
    
    def first_step(self):
        computed_list = self.precomputed_values.copy()
        random.shuffle(computed_list)
        return computed_list
    
    def receive_response(self, response_set, processed_list):
        self.response_set = response_set
        self.processed_list = processed_list
    
    def final_computation(self):
        encrypted_total = None
        for item in self.processed_list:
            computed_val = self.modular_exponentiation(item[0], self.secret_value)
            if computed_val in self.response_set:
                if encrypted_total is None:
                    encrypted_total = item[1]
                else:
                    encrypted_total += item[1]
        return encrypted_total

class Responder(CryptoBase):
    def __init__(self, credentials_data=None):
        self.credentials_data = credentials_data or {
            ("用户身份验证", 15), 
            ("访问权限", 25), 
            ("系统登录", 75)
        }
        self.secret_value = self.create_secret_key()
        self.public_key, self.private_key = self.create_crypto_keys()
        self.hashed_credentials = {cred[0]: self.credential_to_int(cred[0]) 
                                  for cred in self.credentials_data}
        self.precomputed_vals = {
            cred: self.modular_exponentiation(h, self.secret_value) 
            for cred, h in self.hashed_credentials.items()
        }
    
    def process_request(self, request_list):
        self.request_list = request_list
    
    def prepare_response(self):
        response_set = [self.modular_exponentiation(i, self.secret_value) 
                       for i in self.request_list]
        random.shuffle(response_set)
        
        processed_data = []
        for cred_text, value in self.credentials_data:
            computed_hash = self.precomputed_vals[cred_text]
            encrypted_value = self.encrypt_data(value, self.public_key)
            processed_data.append((computed_hash, encrypted_value))
        
        random.shuffle(processed_data)
        return set(response_set), processed_data
    
    def interpret_result(self, encrypted_result):
        return 0 if encrypted_result is None else self.decrypt_data(encrypted_result, self.private_key)

def execute_protocol():
    CryptoBase.initialize()
    initiator = Initiator()
    responder = Responder()
    
    step1_result = initiator.first_step()
    responder.process_request(step1_result)
    
    response_set, processed_list = responder.prepare_response()
    initiator.receive_response(response_set, processed_list)
    
    encrypted_result = initiator.final_computation()
    final_value = responder.interpret_result(encrypted_result)
    
    return initiator.user_credentials, responder.credentials_data, final_value

if __name__ == "__main__":
    creds1, creds2, result = execute_protocol()
    print("发起方凭证:", creds1)
    print("响应方凭证:", creds2)
    print("计算结果:", result)
