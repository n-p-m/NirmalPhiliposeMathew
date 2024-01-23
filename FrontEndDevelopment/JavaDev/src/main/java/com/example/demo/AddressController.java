//this is server side coding. 

package com.example.demo;

import java.util.List;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.PutMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import jakarta.websocket.server.PathParam;

@RestController
@RequestMapping(path = "/addresses")
public class AddressController {
    @Autowired
    private AddressService addressService;

    // we get recorder from database and return it to the client side.
    @GetMapping
    public List<Address> getAll() {
        return addressService.getAll();
    }

    // below method is used to save the data to the database.
    @PostMapping
    public Address createAddress(@RequestBody Address address) {
        return addressService.create(address);
    }

    @PutMapping
    public Address updateAddress(@RequestBody Address address) {
        return addressService.update(address);
    }
@DeleteMapping(path = "/{id}")
public Address deleteAddress(@PathVariable String id  ) {
    // Address address = addressService.get(id);
    return addressService.delete(id);
    
}

}
