package com.example.demo;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

import org.springframework.stereotype.Service;

@Service
public class AddressService {

    private Map<String, Address> addresses = new HashMap<>();

    public Address create(Address address) {
        address.setId(UUID.randomUUID().toString());
        // TODO Auto-generated method stub
        addresses.put(address.getId(), address);
        // throw new UnsupportedOperationException("Unimplemented method 'save'");
        return address;

    }

    public List<Address> getAll() {
        // TODO Auto-generated method stub
        // throw new UnsupportedOperationException("Unimplemented method 'getAll'");
        return new ArrayList<Address>(addresses.values());

    }

    public Address update(Address address) {
        // TODO Auto-generated method stub
        // throw new UnsupportedOperationException("Unimplemented method 'update'");
        String id = address.getId();
        addresses.put(id, address);
        return address;
    }

    public Address get(String id) {
        // TODO Auto-generated method stub
        // throw new UnsupportedOperationException("Unimplemented method 'delete'");
        return addresses.get(id);
    
    }

    public Address delete(String id) {
        // TODO Auto-generated method stub
        // throw new UnsupportedOperationException("Unimplemented method 'delete'");
        return addresses.remove(id);
    }

}
