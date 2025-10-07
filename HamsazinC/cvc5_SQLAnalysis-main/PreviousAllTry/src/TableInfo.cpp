#include "../header/TableInfo.h"

std::vector<std::pair<std::string, cvc5::Sort>> TableInfo::getColumnInfo(const std::string& tableName) {
    std::vector<std::pair<std::string, cvc5::Sort>> columnInfo;

    cvc5::Solver slv;

    if (tableName == "customer") {
        columnInfo.emplace_back("c_id", slv.getIntegerSort());
        columnInfo.emplace_back("c_tax_id", slv.getStringSort());
        columnInfo.emplace_back("c_st_id", slv.getStringSort());
        columnInfo.emplace_back("c_l_name", slv.getStringSort());
        columnInfo.emplace_back("c_f_name", slv.getStringSort());
        columnInfo.emplace_back("c_m_name", slv.getStringSort());
        columnInfo.emplace_back("c_gndr", slv.getStringSort());
        columnInfo.emplace_back("c_tier", slv.getIntegerSort());
        columnInfo.emplace_back("c_dob", slv.getStringSort());
        columnInfo.emplace_back("c_ad_id", slv.getIntegerSort());
        columnInfo.emplace_back("c_ctry_1", slv.getStringSort());
        columnInfo.emplace_back("c_area_1", slv.getStringSort());
        columnInfo.emplace_back("c_local_1", slv.getStringSort());
        columnInfo.emplace_back("c_ext_1", slv.getStringSort());
        columnInfo.emplace_back("c_ctry_2", slv.getStringSort());
        columnInfo.emplace_back("c_area_2", slv.getStringSort());
        columnInfo.emplace_back("c_local_2", slv.getStringSort());
        columnInfo.emplace_back("c_ext_2", slv.getStringSort());
        columnInfo.emplace_back("c_ctry_3", slv.getStringSort());
        columnInfo.emplace_back("c_area_3", slv.getStringSort());
        columnInfo.emplace_back("c_local_3", slv.getStringSort());
        columnInfo.emplace_back("c_ext_3", slv.getStringSort());
        columnInfo.emplace_back("c_email_1", slv.getStringSort());
        columnInfo.emplace_back("c_email_2", slv.getStringSort());
    }
    else if (tableName == "address") {
        columnInfo.emplace_back("ad_id", slv.getIntegerSort());
        columnInfo.emplace_back("ad_line1", slv.getStringSort());
        columnInfo.emplace_back("ad_line2", slv.getStringSort());
        columnInfo.emplace_back("ad_zc_code", slv.getStringSort());
        columnInfo.emplace_back("ad_ctry", slv.getStringSort());
    }
    else if (tableName == "broker") {
        columnInfo.emplace_back("b_id", slv.getIntegerSort());
        columnInfo.emplace_back("b_st_id", slv.getStringSort());
        columnInfo.emplace_back("b_name", slv.getStringSort());
        columnInfo.emplace_back("b_num_trades", slv.getIntegerSort());
        columnInfo.emplace_back("b_comm_total", slv.getRealSort());
    }
    else if(tableName == "zip_code") {
        columnInfo.emplace_back("zc_code", slv.getStringSort());
        columnInfo.emplace_back("zc_town", slv.getStringSort());
        columnInfo.emplace_back("zc_div", slv.getStringSort());
    }
    else if (tableName == "company") {
        columnInfo.emplace_back("co_id", slv.getIntegerSort());
        columnInfo.emplace_back("co_st_id", slv.getStringSort());
        columnInfo.emplace_back("co_name", slv.getStringSort());
        columnInfo.emplace_back("co_in_id", slv.getStringSort());
        columnInfo.emplace_back("co_sp_rate", slv.getStringSort());
        columnInfo.emplace_back("co_ceo", slv.getStringSort());
        columnInfo.emplace_back("co_ad_id", slv.getIntegerSort());
        columnInfo.emplace_back("co_desc", slv.getStringSort());
        columnInfo.emplace_back("co_open_date", slv.getStringSort());
    }

    return columnInfo;
}
