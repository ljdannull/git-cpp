import os
import urllib.request as request
import zlib
import hashlib

target_dir = "./stupid"
url = "https://github.com/codecrafters-io/git-sample-3"

# os.mkdir(target_dir)
# os.mkdir(target_dir + "/.git")
# os.mkdir(target_dir + "/.git/objects/")
# os.mkdir(target_dir + "/.git/refs")
# with open(target_dir + "/.git/HEAD", "w") as f:
#     f.write("ref: refs/heads/master\n")
# print(url, target_dir)
resp = request.urlopen(url + "/info/refs?service=git-upload-pack")
content = resp.read()
resp.close()
# print(content)
resp_as_arr = content.split(b"\n")
# print(resp_as_arr)
for c in resp_as_arr:
    if b"refs/heads/master" in c and b"003f" in c:
        tup = c.split(b" ")
        pack_hash = tup[0][4:].decode()  # convert later
        # print(pack_hash)
post_url = url + "/git-upload-pack"
req = request.Request(post_url)
req.add_header("Content-Type", "application/x-git-upload-pack-request")
data = f"0032want {pack_hash}\n00000009done\n".encode()
pack_resp = request.urlopen(req, data=data)
#print(pack_resp.status)
pack_resp = pack_resp.read()
# return
entries_bytes = pack_resp[16:20]
num_entries = int.from_bytes(entries_bytes, byteorder="big")
#print("entries count", num_entries)
data = pack_resp[20:-20]
objs = {}
seek = 0
objs_count = 0
#for i in data:
#    print("[33m[your_program] [0m" + bin(i)[2:].zfill(8))
while objs_count != num_entries:
    objs_count += 1
    first = data[seek]
    obj_type = (first & 112) >> 4
    # print('obj_type: ', obj_type)
    # num_entries -= 1
    while data[seek] > 128:
        seek += 1
    seek += 1
    #print(seek)
    if obj_type < 7:
        # for i in range(20):
        #     print(bin(data[i]))
        content = zlib.decompress(data[seek:seek+160])
        print(content)
        obj_type_to_str = {1: "commit", 2: "tree", 3: "blob"}
        obj_write_data = (
            f"{obj_type_to_str[obj_type]} {len(content)}\0".encode()
        )
        obj_write_data += content
        # commit_hash = hashlib.sha1(obj_write_data).hexdigest()
        # f_path = target_dir + f"/.git/objects/{commit_hash[:2]}"
        # if not os.path.exists(f_path):
        #     os.mkdir(f_path)
        # with open(
        #     target_dir + f"/.git/objects/{commit_hash[:2]}/{commit_hash[2:]}",
        #     "wb",
        # ) as f:
        #     f.write(zlib.compress(obj_write_data))
        # objs[commit_hash] = (content, obj_type)
        compressed_len = zlib.compress(content)
        #print(len(compressed_len))
        break
        seek += len(compressed_len)
    else:
        # num_entries -= 1
        # 20 byte header for sha1 base reference
        k = data[seek : seek + 20]
        #print(k.hex())
        obs_elem = objs[k.hex()]
        base = obs_elem[0]
        seek += 20
        delta = zlib.decompress(data[seek:])
        compressed_data = zlib.compress(delta)
        content = undeltify(delta, base)
        # print(content)
        obj_type = obs_elem[1]
        obj_type_to_str = {1: "commit", 2: "tree", 3: "blob"}
        obj_write_data = (
            f"{obj_type_to_str[obj_type]} {len(content)}\0".encode()
        )
        obj_write_data += content
        commit_hash = hashlib.sha1(obj_write_data).hexdigest()
        f_path = target_dir + f"/.git/objects/{commit_hash[:2]}"
        if not os.path.exists(f_path):
            os.mkdir(f_path)
        with open(
            target_dir + f"/.git/objects/{commit_hash[:2]}/{commit_hash[2:]}",
            "wb",
        ) as f:
            f.write(zlib.compress(obj_write_data))
        objs[commit_hash] = (content, obj_type)
        seek += len(compressed_data)
