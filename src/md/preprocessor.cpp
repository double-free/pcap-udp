#include "preprocessor.h"
#include "utils.h"

using namespace md;

int MdPreprocessor::process(const u_char *udp_payload)
{
    // PayloadHeader is dropped
    const u_char *message = try_to_construct_message(udp_payload);
    if (message == nullptr)
    {
        return 0;
    }

    // uncompress if needed
    // MessageHeader is dropped
    const u_char *raw_md = uncompress_message(message);
    if (raw_md == nullptr)
    {
        return 0;
    }

    const MessageHeader *header = reinterpret_cast<const MessageHeader *>(message);

    // Debug
    // std::cout << "try to handle message with size " << header->size_before_compress() << ": ";
    // print_hex_array(raw_md, header->size_before_compress());

    md_handler_(raw_md, header->size_before_compress());
    return 1;
}

// return the message constructed after receiving the data
// can be null
const u_char *MdPreprocessor::try_to_construct_message(const u_char *data)
{
    if (data == nullptr)
    {
        return nullptr;
    }

    auto *payload = reinterpret_cast<const PayloadHeader *>(data);
    if (payload->total_packet_number() == 1)
    {
        return data + sizeof(PayloadHeader);
    }

    // it is splitted in several packets
    save_to_cache(payload);

    const u_char *concat_message = construct_message(cache_[payload->channel_id()], payload->sequence_id());

    return concat_message;
}

const u_char *MdPreprocessor::uncompress_message(const u_char *message)
{
    if (message == nullptr)
    {
        return nullptr;
    }

    const auto *header = reinterpret_cast<const MessageHeader *>(message);

    if (header->compressed() == false)
    {
        // not compressed
        return message + sizeof(MessageHeader);
    }

    // uncompress
    size_t decompressed_size = header->size_before_compress();

    // TODO: avoid allocating in critical path
    decompressed_message_.reset(new u_char[decompressed_size]);

    int result = uncompress(decompressed_message_.get(), &decompressed_size, message + sizeof(MessageHeader), header->size_after_compress());
    assert(decompressed_size == header->size_before_compress());

    if (result != Z_OK)
    {
        std::cerr << "uncompress failed with code " << result << ", skip this message" << '\n';
        return nullptr;
    }

    return decompressed_message_.get();
}

void MdPreprocessor::save_to_cache(const PayloadHeader *payload)
{
    if (payload->body_size() > 1500)
    {
        std::cerr << "packet size too big: " << payload->body_size() << '\n';
        std::cerr << "sequence id: " << payload->sequence_id() << '\n';
        md::print_hex_array(reinterpret_cast<const u_char *>(payload), 128);
    }

    auto &per_channel_cache = cache_[payload->channel_id()];
    if (per_channel_cache[payload->sequence_id()].empty())
    {
        per_channel_cache[payload->sequence_id()].resize(
            payload->total_packet_number());
    }

    uint32_t packet_index =
        payload->current_packet_index() + payload->initial_packet_index();
    auto &char_vec = per_channel_cache[payload->sequence_id()][packet_index];
    char_vec.reserve(payload->body_size());

    // copy to make sure message staying valid
    const u_char *body =
        reinterpret_cast<const u_char *>(payload) + sizeof(PayloadHeader);

    for (int i = 0; i < payload->body_size(); i++)
    {
        char_vec.push_back(body[i]);
    }
}

const u_char *MdPreprocessor::construct_message(Cache &cache, uint64_t seq_id)
{
    if (cache.find(seq_id) == cache.end())
    {
        return nullptr;
    }

    const auto &char_vecs = cache[seq_id];

    bool received = std::all_of(
        char_vecs.begin(), char_vecs.end(),
        [](const std::vector<u_char> &vec)
        { return vec.empty() == false; });
    if (received == false)
    {
        return nullptr;
    }

    int length = 0;
    std::for_each(
        char_vecs.begin(), char_vecs.end(),
        [&length](const std::vector<u_char> &vec)
        { length += vec.size(); });

    concatenated_message_.reset(new u_char[length]);
    u_char *buffer = concatenated_message_.get();

    int index = 0;
    for (const auto &char_vec : char_vecs)
    {
        for (u_char c : char_vec)
        {
            buffer[index] = c;
            ++index;
        }
    }
    // no longer used
    cache.erase(seq_id);

    return buffer;
}